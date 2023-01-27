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
	_Atomic(simtime_t) window_last;
} racer = {.window_upper = SIMTIME_MAX, .window_last = 0};

simtime_t racer_last(void)
{
	return atomic_load_explicit(&racer.window_last, memory_order_relaxed);
}

static void racer_on_rollback(simtime_t t)
{
	simtime_t w = atomic_load_explicit(&racer.window_upper, memory_order_relaxed);
	while(unlikely(t < w && !atomic_compare_exchange_weak_explicit(&racer.window_upper, &w, t, memory_order_relaxed,
				    memory_order_relaxed)))
		spin_pause();
}

static void racer_align_lps(void)
{
	simtime_t w = atomic_load_explicit(&racer.window_upper, memory_order_relaxed);
	for(uint64_t i = lid_thread_first; i < lid_thread_end; ++i) {
		struct lp_ctx *lp = &lps[i];

		if(w <= lp->p.bound) {
			array_count_t past_i = match_straggler_time(&lp->p, w);
			current_lp = lp;
			do_rollback(lp, past_i);
			termination_on_lp_rollback(lp, w);
			lp->p.bound = unlikely(array_is_empty(lp->p.p_msgs)) ? -1.0 : array_peek(lp->p.p_msgs)->dest_t;
		}
	}
}

static inline void racer_window_rearm(void)
{
	static simtime_t window_delta;
	simtime_t w = atomic_load_explicit(&racer.window_upper, memory_order_relaxed);
	window_delta = EXP_AVG(8, window_delta, DELTA_EXTENSION * (w - racer.window_last));
	atomic_store_explicit(&racer.window_last, w, memory_order_relaxed);
	atomic_store_explicit(&racer.window_upper, w + window_delta, memory_order_relaxed);
}

static void racer_window_commit(void)
{
	unsigned r;

	static __thread unsigned phase;
	static atomic_uint cs[2];
	atomic_uint *c = cs + (phase & 1U);
	bool inc = phase & 2U;
	phase = (phase + 1) & 3U;
	if(inc) {
		atomic_fetch_add_explicit(c, -1, memory_order_acq_rel);
		do {
			r = atomic_load_explicit(c, memory_order_acquire);
			racer_align_lps();
		} while(r);
	} else {
		rid_t thr_cnt = global_config.n_threads_racer;
		atomic_fetch_add_explicit(c, 1, memory_order_acq_rel);
		do {
			r = atomic_load_explicit(c, memory_order_acquire);
			racer_align_lps();
		} while(r != thr_cnt);
	}

	if(sync_thread_barrier_racer())
		racer_window_rearm();

	sync_thread_barrier_racer();

	simtime_t s = racer_last();
	termination_on_gvt(s);
	auto_ckpt_on_gvt();
	fossil_on_gvt(s);
	msg_allocator_on_gvt(s);
	stats_on_gvt(s);
}

void racer_process_msg(void)
{
	struct lp_msg *msg = msg_queue_extract();
	if(unlikely(!msg)) {
		racer_window_commit();
		return;
	}

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
		msg_queue_insert(msg);
		racer_window_commit();
		return;
	}

	if(unlikely(atomic_load_explicit(&racer.window_upper, memory_order_relaxed) <= msg->dest_t)) {
		msg_queue_insert(msg);
		racer_window_commit();
		return;
	}

#ifndef NDEBUG
	current_msg = msg;
#endif

	common_msg_process(lp, msg);
	lp->p.bound = msg->dest_t;
	array_push(lp->p.p_msgs, msg);

	auto_ckpt_register_good(&lp->auto_ckpt);
	if(auto_ckpt_is_needed(&lp->auto_ckpt))
		checkpoint_take(lp);

	termination_on_msg_process(lp, msg->dest_t);
}
