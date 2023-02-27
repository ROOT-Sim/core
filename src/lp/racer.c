#include <lp/process.c>

#define EXP_AVG(f, old_v, sample)                                                                                      \
	__extension__({                                                                                                \
		double s = (sample);                                                                                   \
		double o = (old_v);                                                                                    \
		o *(((f)-1.0) / (f)) + s * (1.0 / (f));                                                                \
	})

#define DELTA_EXTENSION 1.1

static struct {
	_Atomic(simtime_t) window_upper;
	simtime_t window_last;
} racer = {.window_upper = SIMTIME_MAX, .window_last = 0};

static void racer_on_rollback(simtime_t t)
{
	simtime_t w = atomic_load_explicit(&racer.window_upper, memory_order_relaxed);
	while(unlikely(t < w && !atomic_compare_exchange_weak_explicit(&racer.window_upper, &w, t, memory_order_relaxed,
				    memory_order_relaxed)))
		spin_pause();
}

__attribute__((unused)) static void racer_align_lps(simtime_t w)
{
	for(uint64_t i = lid_thread_first; i < lid_thread_end; ++i) {
		struct lp_ctx *lp = &lps[i];

		if(w <= lp->p.bound) {
			array_count_t past_i = match_straggler_time(&lp->p, w);
			current_lp = lp;
			do_rollback(lp, past_i);
			termination_on_lp_rollback(lp, w);
			lp->p.bound = unlikely(array_is_empty(lp->p.p_msgs)) ? -1.0 : array_peek(lp->p.p_msgs)->dest_t;
			retractable_reschedule(lp);
		}
	}
}

static inline void racer_window_rearm(void)
{
	static simtime_t window_delta;
	simtime_t w = atomic_load_explicit(&racer.window_upper, memory_order_relaxed);
	if(w > racer.window_last)
		window_delta = EXP_AVG(4, window_delta, DELTA_EXTENSION * (w - racer.window_last));
	racer.window_last = w;
	atomic_store_explicit(&racer.window_upper, w + window_delta, memory_order_relaxed);
}

static void racer_window_commit(void)
{
	static atomic_uint cs[2];
	static __thread bool racer_ready;
	static __thread unsigned racer_phase;
	unsigned dec = 1 - racer_phase;
	unsigned cmp = racer_phase ? 0 : global_config.n_threads_racer;
	if(!racer_ready) {
		atomic_fetch_add_explicit(&cs[0], dec, memory_order_relaxed);
		racer_ready = true;
	}

	rid_t v = atomic_load_explicit(&cs[0], memory_order_relaxed);
	if(v != cmp)
		return;

	racer_phase ^= 2U;
	racer_ready = false;
	if(rid == 0)
		racer_window_rearm();

	v = atomic_fetch_add_explicit(&cs[1], dec, memory_order_acq_rel) + dec;
	while(v != cmp && likely(termination_cant_end())) {
		spin_pause();
		v = atomic_load_explicit(&cs[1], memory_order_relaxed);
	}
}

void racer_process_msg(void)
{
	struct lp_msg *msg = msg_queue_extract();
	if(unlikely(!msg)) {
		racer_window_commit();
		return;
	}

	gvt_on_msg_extraction(msg->dest_t);

	struct lp_ctx *lp = &lps[msg->dest];
	current_lp = lp;

	if(unlikely(fossil_is_needed(lp))) {
		auto_ckpt_recompute(&lp->auto_ckpt, lp->mm_state.full_ckpt_size);
		fossil_lp_collect(lp);
		lp->p.bound = unlikely(array_is_empty(lp->p.p_msgs)) ? -1.0 : lp->p.bound;
	}

	uint32_t flags = atomic_fetch_add_explicit(&msg->flags, MSG_FLAG_PROCESSED, memory_order_relaxed);
	if(unlikely(flags & MSG_FLAG_ANTI)) {
		if(unlikely(flags < (MSG_FLAG_ANTI | MSG_FLAG_PROCESSED))) {
			msg_allocator_free(msg);
		} else {
			racer_on_rollback(msg->dest_t);
			handle_anti_msg(lp, msg, flags > (MSG_FLAG_ANTI | MSG_FLAG_PROCESSED));
			lp->p.bound = unlikely(array_is_empty(lp->p.p_msgs)) ? -1.0 : array_peek(lp->p.p_msgs)->dest_t;
			retractable_reschedule(lp);
			racer_window_commit();
		}
		return;
	}

	if(unlikely(flags && lp->p.early_antis && check_early_anti_messages(&lp->p, msg)))
		return;

	if(unlikely(lp->p.bound >= msg->dest_t && msg_is_before(msg, array_peek(lp->p.p_msgs)))) {
		racer_on_rollback(msg->dest_t);
		handle_straggler_msg(lp, msg);
		lp->p.bound = unlikely(array_is_empty(lp->p.p_msgs)) ? -1.0 : array_peek(lp->p.p_msgs)->dest_t;
		uint32_t f = atomic_fetch_add_explicit(&msg->flags, -MSG_FLAG_PROCESSED, memory_order_relaxed);
		if(!(f & MSG_FLAG_ANTI))
			msg_queue_insert_own(msg);
		retractable_reschedule(lp);
		racer_window_commit();
		return;
	}

	if(unlikely(atomic_load_explicit(&racer.window_upper, memory_order_relaxed) <= msg->dest_t)) {
		uint32_t f = atomic_fetch_add_explicit(&msg->flags, -MSG_FLAG_PROCESSED, memory_order_relaxed);
		if(!(f & MSG_FLAG_ANTI)) {
			if(is_retractable(msg)) {
				msg_allocator_free(msg);
				ScheduleRetractableEvent(msg->dest_t);
				retractable_reschedule(lp);
			} else {
				msg_queue_insert_own(msg);
			}
		}
		racer_window_commit();
		return;
	}

#ifndef NDEBUG
	current_msg = msg;
#endif

	common_msg_process(lp, msg);
	lp->p.bound = msg->dest_t;
	retractable_reschedule(lp);
	array_push(lp->p.p_msgs, msg);

	auto_ckpt_register_good(&lp->auto_ckpt);
	if(auto_ckpt_is_needed(&lp->auto_ckpt))
		checkpoint_take(lp);

	termination_on_msg_process(lp, msg->dest_t);
}
