#include <lp/racer.h>

#include <arch/timer.h>
#include <core/sync.h>
#include <datatypes/heap.h>
#include <datatypes/msg_queue.h>
#include <distributed/mpi.h>
#include <gvt/fossil.h>
#include <log/stats.h>
#include <lp/common.h>
#include <lp/lp.h>
#include <mm/msg_allocator.h>
#include <serial/serial.h>

#include <stdatomic.h>

#define s_elem_is_before(ma, mb) ((ma).t < (mb).t || ((ma).t == (mb).t && msg_is_before_extended(ma.m, mb.m)))

/// An element in the message queue
struct s_elem {
	/// The timestamp of the message
	simtime_t t;
	/// The message enqueued
	struct lp_msg *m;
};

static _Atomic(simtime_t) window_ub;
static _Atomic(rid_t) read_counter;

static __thread dyn_array(struct lp_ctx *) dirties;
static __thread heap_declare(struct s_elem) speculative;

#ifndef NDEBUG
/// The currently processed message
/** This is not necessary for normal operation, but it's useful in debug */
extern __thread struct lp_msg *current_msg;
#endif

/**
 * @brief Take a checkpoint of the state of a LP
 * @param lp the LP to checkpoint
 *
 * The actual checkpoint operation is delegated to the model memory allocator.
 */
static inline void checkpoint_take(struct lp_ctx *lp)
{
	timer_uint t = timer_hr_new();
	model_allocator_checkpoint_take(&lp->mm_state, array_count(lp->p.p_msgs));
	stats_take(STATS_CKPT_SIZE, lp->mm_state.full_ckpt_size);
	stats_take(STATS_CKPT, 1);
	stats_take(STATS_CKPT_TIME, timer_hr_value(t));
}

static inline void msg_invalidate(struct lp_msg *msg)
{
	if(msg->raw_flags == MSG_FLAG_PROCESSED)
		msg_queue_insert(msg);
	else {
		struct s_elem se = {.t = msg->dest_t, .m = msg};
		heap_insert(speculative, s_elem_is_before, se);
	}
}

static inline void msgs_invalidate(struct process_ctx *proc_p, array_count_t past_i)
{
	array_count_t p_cnt = array_count(proc_p->p_msgs);
	for(array_count_t i = past_i; i < p_cnt; ++i)
		msg_invalidate(array_get_at(proc_p->p_msgs, i));

	array_count(proc_p->p_msgs) = past_i;
}

static inline void racer_fossil_collect(struct lp_ctx *lp)
{
	array_count_t ref_i = model_allocator_fossil_lp_collect(&lp->mm_state, array_count(lp->p.p_msgs));
	for(array_count_t i = 0; i < ref_i; ++i)
		msg_allocator_free(array_get_at(lp->p.p_msgs, i));

	array_truncate_first(lp->p.p_msgs, ref_i);
}

/**
 * @brief Perform silent execution of events
 * @param proc_p the message processing data for the LP that has to coast forward
 * @param last_i the index in @a proc_p of the last processed message in the current LP state
 * @param past_i the target index in @a proc_p of the message to reach with the silent execution operation
 *
 * This function implements the coasting forward operation done after a checkpoint has been restored.
 */
static inline void silent_execution(const struct lp_ctx *lp, array_count_t last_i, array_count_t past_i)
{
	if(unlikely(last_i >= past_i))
		return;

	timer_uint t = timer_hr_new();
	silent_processing = true;

	void *state_p = lp->state_pointer;
	do {
		const struct lp_msg *msg = array_get_at(lp->p.p_msgs, last_i);
		global_config.dispatcher(msg->dest, msg->dest_t, msg->m_type, msg->pl, msg->pl_size, state_p);
		stats_take(STATS_MSG_SILENT, 1);
	} while(++last_i < past_i);

	silent_processing = false;
	stats_take(STATS_MSG_SILENT_TIME, timer_hr_value(t));
}

/**
 * @brief Perform a rollback
 * @param proc_p the message processing data for the LP that has to rollback
 * @param past_i the index in @a proc_p of the last validly processed message
 */
static void do_rollback(struct lp_ctx *lp, array_count_t past_i)
{
	timer_uint t = timer_hr_new();
	msgs_invalidate(&lp->p, past_i);
	array_count_t last_i = model_allocator_checkpoint_restore(&lp->mm_state, past_i);
	stats_take(STATS_RECOVERY_TIME, timer_hr_value(t));
	stats_take(STATS_ROLLBACK, 1);
	silent_execution(lp, last_i, past_i);
	auto_ckpt_register_bad(&lp->auto_ckpt);
	lp->p.bound = array_count(lp->p.p_msgs) ? array_peek(lp->p.p_msgs)->dest_t : 0.0;
}

/**
 * @brief Find the last valid processed message with respect to a straggler message
 * @param proc_p the message processing data for the LP
 * @param s_msg the straggler message
 * @return the index in @a proc_p of the last validly processed message
 */
static inline array_count_t match_straggler_msg(const struct process_ctx *proc_p, const struct lp_msg *s_msg)
{
	array_count_t i = array_count(proc_p->p_msgs) - 1;
	while(i) {
		const struct lp_msg *msg = array_get_at(proc_p->p_msgs, --i);
		if(!msg_is_before(s_msg, msg))
			return i + 1;
	}
	return 0;
}

static inline array_count_t match_straggler_time(const struct process_ctx *proc_p, simtime_t t)
{
	array_count_t i = array_count(proc_p->p_msgs) - 1;
	while(i) {
		const struct lp_msg *msg = array_get_at(proc_p->p_msgs, --i);
		if(msg->dest_t < t)
			return i + 1;
	}
	return 0;
}

static void handle_straggler(struct lp_ctx *lp, const struct lp_msg *msg)
{
	simtime_t t = msg->dest_t;
	simtime_t w = atomic_load_explicit(&window_ub, memory_order_relaxed);
	while(unlikely(t < w && !atomic_compare_exchange_weak_explicit(&window_ub, &w, t, memory_order_relaxed,
				    memory_order_relaxed)))
		spin_pause();

	array_count_t past_i = match_straggler_msg(&lp->p, msg);
	do_rollback(lp, past_i);
}

static void handle_straggler_time(struct lp_ctx *lp, simtime_t t)
{
	array_count_t past_i = match_straggler_time(&lp->p, t);
	current_lp = lp;
	do_rollback(lp, past_i);
}

static inline bool racer_process_message(struct lp_msg *msg)
{
	struct lp_ctx *lp = &lps[msg->dest];
	current_lp = lp;

	spin_lock(&lp->lock);

	if(!lp->dirty) {
		lp->dirty = true;
		array_push(dirties, lp);
	}

	if(unlikely(lp->p.bound >= msg->dest_t && msg_is_before(msg, array_peek(lp->p.p_msgs)))) {
		handle_straggler(lp, msg);
		spin_unlock(&lp->lock);
		msg_invalidate(msg);
		return true;
	}

#ifndef NDEBUG
	current_msg = msg;
#endif

	lp->p.bound = msg->dest_t;
	common_msg_process(lp, msg);
	array_push(lp->p.p_msgs, msg);

	auto_ckpt_register_good(&lp->auto_ckpt);
	if(auto_ckpt_is_needed(&lp->auto_ckpt))
		checkpoint_take(lp);

	spin_unlock(&lp->lock);
	return false;
}

static inline void racer_process_messages(void)
{
	atomic_fetch_add_explicit(&read_counter, 1, memory_order_relaxed);
	msg_queue_update();
	do {
		simtime_t queue_min = msg_queue_time_peek_nf();
		while(heap_count(speculative) && heap_min(speculative).t < queue_min)
			if(racer_process_message(heap_extract(speculative, s_elem_is_before).m))
				return;

		if(queue_min >= atomic_load_explicit(&window_ub, memory_order_relaxed))
		    return;

		struct lp_msg *msg = msg_queue_extract_nf();
		if(racer_process_message(msg))
			return;
	} while(true);
}

static inline void flush_valid_speculative_sent(simtime_t gvt)
{
	for(array_count_t i = 0; i < array_count(speculative); ++i) {
		struct lp_msg *msg = array_get_at(speculative, i).m;
		if(msg->racer_send_t >= gvt) {
			msg_allocator_free(msg);
		} else {
			msg->raw_flags = MSG_FLAG_PROCESSED;
			msg_queue_insert(msg);
		}
	}
	array_count(speculative) = 0;
}

static inline void racer_fossil(void)
{
	simtime_t window = atomic_load_explicit(&window_ub, memory_order_relaxed);
	if(atomic_fetch_sub_explicit(&read_counter, 1, memory_order_acq_rel) == 1)
		atomic_store_explicit(&window_ub, SIMTIME_MAX, memory_order_relaxed);

	for(array_count_t i = array_count(dirties); i--;) {
		struct lp_ctx *lp = array_get_at(dirties, i);
		if(unlikely(window <= lp->p.bound))
			handle_straggler_time(lp, window);

		auto_ckpt_register_good(&lp->auto_ckpt);
		auto_ckpt_recompute(&lp->auto_ckpt, lp->mm_state.full_ckpt_size);
		racer_fossil_collect(lp);
		termination_lp_on_window(lp);

		lp->dirty = false;
	}
	array_count(dirties) = 0;
	termination_on_window(window);
	flush_valid_speculative_sent(window);
	auto_ckpt_on_gvt();
	stats_on_gvt(window);
}

void racer_msg_insert(struct lp_msg *msg)
{
	msg->racer_send_t = current_lp->p.bound;
	msg->raw_flags = 0;
	struct s_elem se = {.t = msg->dest_t, .m = msg};
	heap_insert(speculative, s_elem_is_before, se);
}

void racer_init(void)
{
	heap_init(speculative);
	array_init(dirties);
	atomic_store_explicit(&window_ub, SIMTIME_MAX, memory_order_relaxed);
}

void racer_run(void)
{
	flush_valid_speculative_sent(SIMTIME_MAX);

	while(likely(termination_cant_end())) {
		racer_process_messages();
		sync_thread_barrier();
		racer_fossil();
		sync_thread_barrier();
	}

	array_fini(dirties);
	heap_fini(speculative);
}
