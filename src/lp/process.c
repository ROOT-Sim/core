/**
 * @file lp/process.c
 *
 * @brief LP state management functions
 *
 * This module contains the main logic for the parallel simulation runtime
 *
 * SPDX-FileCopyrightText: 2008-2023 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <lp/process.h>

#include <arch/timer.h>
#include <datatypes/msg_queue.h>
#include <distributed/mpi.h>
#include <gvt/fossil.h>
#include <gvt/gvt.h>
#include <log/stats.h>
#include <lp/common.h>
#include <lp/lp.h>
#include <mm/auto_ckpt.h>
#include <mm/msg_allocator.h>
#include <serial/serial.h>

/// The flag used in ScheduleNewEvent() to keep track of silent execution
static __thread bool silent_processing = false;
#ifndef NDEBUG
/// The currently processed message
/** This is not necessary for normal operation, but it's useful in debug */
static __thread struct lp_msg *current_msg;
#endif

#define mark_msg_remote(msg_p) ((struct lp_msg *)(((uintptr_t)(msg_p)) | 2U))
#define mark_msg_sent(msg_p) ((struct lp_msg *)(((uintptr_t)(msg_p)) | 1U))
#define unmark_msg_remote(msg_p) ((struct lp_msg *)(((uintptr_t)(msg_p)) - 2U))
#define unmark_msg_sent(msg_p) ((struct lp_msg *)(((uintptr_t)(msg_p)) - 1U))

void ScheduleNewEvent(lp_id_t receiver, simtime_t timestamp, unsigned event_type, const void *payload,
    unsigned payload_size)
{
	if(unlikely(global_config.serial)) {
		ScheduleNewEvent_serial(receiver, timestamp, event_type, payload, payload_size);
		return;
	}

	if(unlikely(silent_processing))
		return;

	struct lp_msg *msg = msg_allocator_pack(receiver, timestamp, event_type, payload, payload_size);

#ifndef NDEBUG
	msg->raw_flags = 0;
	if(msg_is_before(msg, current_msg)) {
		logger(LOG_FATAL, "Scheduling a message in the past!");
		abort();
	}
	msg->send = current_lp - lps;
	msg->send_t = current_msg->dest_t;
#endif

	nid_t dest_nid = lid_to_nid(receiver);
	if(dest_nid != nid) {
		mpi_remote_msg_send(msg, dest_nid);
		array_push(current_lp->p.p_msgs, mark_msg_remote(msg));
	} else {
		atomic_store_explicit(&msg->flags, 0U, memory_order_relaxed);
		msg_queue_insert(msg);
		array_push(current_lp->p.p_msgs, mark_msg_sent(msg));
	}
}

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

/**
 * @brief Initializes the processing module in the current LP
 */
void process_lp_init(struct lp_ctx *lp)
{
	array_init(lp->p.p_msgs);
	lp->p.early_antis = NULL;

	struct lp_msg *msg = msg_allocator_pack(lp - lps, 0, LP_INIT, NULL, 0U);
	msg->raw_flags = MSG_FLAG_PROCESSED;
#ifndef NDEBUG
	current_msg = msg;
#endif
	current_lp = lp;
	common_msg_process(lp, msg);
	lp->p.bound = 0.0;
	array_push(lp->p.p_msgs, msg);
	model_allocator_checkpoint_next_force_full(&lp->mm_state);
	checkpoint_take(lp);
}

/**
 * @brief Finalize the processing module in the current LP
 */
void process_lp_fini(struct lp_ctx *lp)
{
	current_lp = lp;
	global_config.dispatcher(lp - lps, 0, LP_FINI, NULL, 0, lp->state_pointer);

	for(array_count_t i = 0; i < array_count(lp->p.p_msgs); ++i) {
		struct lp_msg *msg = array_get_at(lp->p.p_msgs, i);
		if(is_msg_local_sent(msg))
			continue;

		bool remote = is_msg_remote(msg);
		msg = unmark_msg(msg);
		uint32_t flags = atomic_load_explicit(&msg->flags, memory_order_relaxed);
		if(remote || !(flags & MSG_FLAG_ANTI))
			msg_allocator_free(msg);
	}
	array_fini(lp->p.p_msgs);
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
		while(is_msg_sent(msg))
			msg = array_get_at(lp->p.p_msgs, ++last_i);

		global_config.dispatcher(msg->dest, msg->dest_t, msg->m_type, msg->pl, msg->pl_size, state_p);
		stats_take(STATS_MSG_SILENT, 1);
	} while(++last_i < past_i);

	silent_processing = false;
	stats_take(STATS_MSG_SILENT_TIME, timer_hr_value(t));
}

/**
 * @brief Send anti-messages
 * @param proc_p the message processing data for the LP that has to send anti-messages
 * @param past_i the index in @a proc_p of the last validly processed message
 */
static inline void send_anti_messages(struct process_ctx *proc_p, array_count_t past_i)
{
	array_count_t p_cnt = array_count(proc_p->p_msgs);
	for(array_count_t i = past_i; i < p_cnt; ++i) {
		struct lp_msg *msg = array_get_at(proc_p->p_msgs, i);

		while(is_msg_sent(msg)) {
			if(is_msg_remote(msg)) {
				msg = unmark_msg_remote(msg);
				nid_t dest_nid = lid_to_nid(msg->dest);
				mpi_remote_anti_msg_send(msg, dest_nid);
				msg_allocator_free_at_gvt(msg);
			} else {
				msg = unmark_msg_sent(msg);
				uint32_t f =
				    atomic_fetch_add_explicit(&msg->flags, MSG_FLAG_ANTI, memory_order_relaxed);
				if(f & MSG_FLAG_PROCESSED)
					msg_queue_insert(msg);
			}

			stats_take(STATS_MSG_ANTI, 1);
			msg = array_get_at(proc_p->p_msgs, ++i);
		}

		uint32_t f = atomic_fetch_add_explicit(&msg->flags, -MSG_FLAG_PROCESSED, memory_order_relaxed);
		if(!(f & MSG_FLAG_ANTI))
			msg_queue_insert(msg);
		stats_take(STATS_MSG_ROLLBACK, 1);
	}
	array_count(proc_p->p_msgs) = past_i;
}

/**
 * @brief Perform a rollback
 * @param proc_p the message processing data for the LP that has to rollback
 * @param past_i the index in @a proc_p of the last validly processed message
 */
static void do_rollback(struct lp_ctx *lp, array_count_t past_i)
{
	timer_uint t = timer_hr_new();
	send_anti_messages(&lp->p, past_i);
	array_count_t last_i = model_allocator_checkpoint_restore(&lp->mm_state, past_i);
	stats_take(STATS_RECOVERY_TIME, timer_hr_value(t));
	stats_take(STATS_ROLLBACK, 1);
	silent_execution(lp, last_i, past_i);
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
	const struct lp_msg *msg;
	do {
		if(!i)
			return 0;
		msg = array_get_at(proc_p->p_msgs, --i);
	} while(is_msg_sent(msg) || msg_is_before(s_msg, msg));
	return i + 1;
}

/**
 * @brief Find the last valid processed message with respect to a received anti-message
 * @param proc_p the message processing data for the LP
 * @param a_msg the anti-message
 * @return the index in @a proc_p of the last validly processed message
 */
static inline array_count_t match_anti_msg(const struct process_ctx *proc_p, const struct lp_msg *a_msg)
{
	array_count_t i = array_count(proc_p->p_msgs) - 1;
	const struct lp_msg *msg = array_get_at(proc_p->p_msgs, i);
	while(a_msg != msg)
		msg = array_get_at(proc_p->p_msgs, --i);

	while(i) {
		msg = array_get_at(proc_p->p_msgs, --i);
		if(is_msg_past(msg))
			return i + 1;
	}
	return i;
}

/**
 * @brief Handle the reception of a remote anti-message
 * @param proc_p the message processing data for the LP that has to handle the anti-message
 * @param a_msg the remote anti-message
 */
static inline void handle_remote_anti_msg(struct lp_ctx *lp, struct lp_msg *a_msg)
{
	// Simplifies flags-based matching, also useful in the early remote anti-messages matching
	a_msg->raw_flags -= MSG_FLAG_ANTI;

	uint32_t m_id = a_msg->raw_flags, m_seq = a_msg->m_seq;
	array_count_t i = array_count(lp->p.p_msgs);
	struct lp_msg *msg;
	do {
		if(unlikely(!i)) {
			// Sadly this is an early remote anti-message
			a_msg->next = lp->p.early_antis;
			lp->p.early_antis = a_msg;
			return;
		}
		msg = array_get_at(lp->p.p_msgs, --i);
	} while(is_msg_sent(msg) || msg->raw_flags != m_id || msg->m_seq != m_seq);

	while(i) {
		const struct lp_msg *v_msg = array_get_at(lp->p.p_msgs, --i);
		if(is_msg_past(v_msg)) {
			i++;
			break;
		}
	}

	msg->raw_flags |= MSG_FLAG_ANTI;
	do_rollback(lp, i);
	termination_on_lp_rollback(lp, msg->dest_t);
	msg_allocator_free(msg);
	msg_allocator_free(a_msg);
}

/**
 * @brief Check if a remote message has already been invalidated by an early remote anti-message
 * @param proc_p the message processing data of the current LP
 * @param a_msg the remote message to check
 * @return true if the message has been matched with an early remote anti-message, false otherwise
 */
static inline bool check_early_anti_messages(struct process_ctx *proc_p, struct lp_msg *msg)
{
	uint32_t m_id = msg->raw_flags, m_seq = msg->m_seq;
	struct lp_msg **prev_p = &proc_p->early_antis;
	struct lp_msg *a_msg = *prev_p;
	do {
		if(a_msg->raw_flags == m_id && a_msg->m_seq == m_seq) {
			*prev_p = a_msg->next;
			msg_allocator_free(msg);
			msg_allocator_free(a_msg);
			return true;
		}
		prev_p = &a_msg->next;
		a_msg = *prev_p;
	} while(a_msg != NULL);
	return false;
}

/**
 * @brief Handle the reception of an anti-message
 * @param lp the processing context of the current LP
 * @param msg the received anti-message
 * @param last_flags the original value of the message flags before being modified by the current process_msg() call
 */
static void handle_anti_msg(struct lp_ctx *lp, struct lp_msg *msg, uint32_t last_flags)
{
	if(last_flags > (MSG_FLAG_ANTI | MSG_FLAG_PROCESSED)) {
		handle_remote_anti_msg(lp, msg);
		auto_ckpt_register_bad(&lp->auto_ckpt);
		return;
	} else if(last_flags == (MSG_FLAG_ANTI | MSG_FLAG_PROCESSED)) {
		array_count_t past_i = match_anti_msg(&lp->p, msg);
		do_rollback(lp, past_i);
		termination_on_lp_rollback(lp, msg->dest_t);
		auto_ckpt_register_bad(&lp->auto_ckpt);
	}
	msg_allocator_free(msg);
}

/**
 * @brief Handle the reception of a straggler message
 * @param lp the processing context of the current LP
 * @param msg the received straggler message
 */
static void handle_straggler_msg(struct lp_ctx *lp, struct lp_msg *msg)
{
	array_count_t past_i = match_straggler_msg(&lp->p, msg);
	do_rollback(lp, past_i);
	termination_on_lp_rollback(lp, msg->dest_t);
	auto_ckpt_register_bad(&lp->auto_ckpt);
}

/**
 * @brief Extract and process a message, if available
 *
 * This function encloses most of the actual parallel/distributed simulation logic.
 */
void process_msg(void)
{
	struct lp_msg *msg = msg_queue_extract();
	if(unlikely(!msg)) {
		current_lp = NULL;
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
		handle_anti_msg(lp, msg, flags);
		lp->p.bound = unlikely(array_is_empty(lp->p.p_msgs)) ? -1.0 : lp->p.bound;
		return;
	}

	if(unlikely(flags && lp->p.early_antis && check_early_anti_messages(&lp->p, msg)))
		return;

	if(unlikely(lp->p.bound >= msg->dest_t && msg_is_before(msg, array_peek(lp->p.p_msgs))))
		handle_straggler_msg(lp, msg);

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
