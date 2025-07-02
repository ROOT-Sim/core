/**
 * @file lp/process.c
 *
 * @brief LP state management functions
 *
 * This module contains the main logic for the parallel simulation runtime
 *
 * SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <lp/process.h>

#include <arch/timer.h>
#include <datatypes/msg_queue.h>
#include <distributed/mpi.h>
#include <gvt/fossil.h>
#include <gvt/gvt.h>
#include <gvt/termination.h>
#include <log/stats.h>
#include <lp/common.h>
#include <lp/lp.h>
#include <mm/checkpoint/checkpoint.h>
#include <mm/msg_allocator.h>
#include <serial/serial.h>

#include <assert.h>

/// The flag used in ScheduleNewEvent() to keep track of silent execution
static _Thread_local bool silent_processing = false;

/**
 * @brief Schedule a new event. Parallel (Time Warp) version.
 * @param receiver destination LP
 * @param timestamp timestamp of the injected event
 * @param event_type model-defined type
 * @param payload payload of the event
 * @param payload_size size of the payload
 */
void ScheduleNewEvent_parallel(const lp_id_t receiver, const simtime_t timestamp, const unsigned event_type,
    const void *payload, const unsigned payload_size)
{
	if(unlikely(silent_processing))
		return;

	struct lp_msg *const msg = common_msg_pack(receiver, timestamp, event_type, payload, payload_size);

	const nid_t dest_nid = lid_to_nid(receiver);
	if(dest_nid != nid) {
		mpi_remote_msg_send(msg, dest_nid);
		array_push(current_lp->p.pes, pes_entry_make(msg, PES_ENTRY_SENT_REMOTE));
	} else {
		atomic_store_explicit(&msg->flags, 0U, memory_order_relaxed);
		msg_queue_insert(msg);
		array_push(current_lp->p.pes, pes_entry_make(msg, PES_ENTRY_SENT_LOCAL));
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
	const timer_uint t = timer_hr_new();
	model_allocator_checkpoint_take(&lp->mm_state, array_count(lp->p.pes));
	stats_take(STATS_CKPT_SIZE, lp->mm_state.full_ckpt_size);
	stats_take(STATS_CKPT, 1);
	stats_take(STATS_CKPT_TIME, timer_hr_value(t));
}

/**
 * @brief Initializes the processing module in the current LP
 */
void process_lp_init(struct lp_ctx *lp)
{
	array_init(lp->p.pes);
	lp->p.early_antis = NULL;
	lp->p.bound = -1.0;
	struct lp_msg *const msg = common_msg_pack(lp - lps, 0, LP_INIT, NULL, 0U);
	msg->raw_flags = 0;
	model_allocator_checkpoint_next_force_full(&lp->mm_state);
	process_msg(msg);
}

/**
 * @brief Finalize the processing module in the current LP
 */
void process_lp_fini(struct lp_ctx *lp)
{
	current_lp = lp;
	silent_processing = true;
	global_config.dispatcher(lp - lps, 0, LP_FINI, NULL, 0, lp->state_pointer);

	for(array_count_t i = 0; i < array_count(lp->p.pes); ++i) {
		const struct pes_entry e = array_get_at(lp->p.pes, i);
		if(pes_entry_is_sent_local(e))
			continue;

		if(pes_entry_is_sent_remote(e) ||
		    !(atomic_load_explicit(&pes_entry_msg_received(e)->flags, memory_order_relaxed) & MSG_FLAG_ANTI))
			msg_allocator_free(pes_entry_msg(e));
	}
	array_fini(lp->p.pes);
}

/**
 * @brief Perform silent execution of events
 * @param lp the message processing data for the LP that has to coast forward
 * @param last_i the index in @a proc_p of the last processed message in the current LP state
 * @param past_i the target index in @a proc_p of the message to reach with the silent execution operation
 *
 * This function implements the coasting forward operation done after a checkpoint has been restored.
 */
static inline void silent_execution(const struct lp_ctx *lp, array_count_t last_i, const array_count_t past_i)
{
	if(unlikely(last_i >= past_i))
		return;

	const timer_uint t = timer_hr_new();
	silent_processing = true;

	void *state_p = lp->state_pointer;
	do {
		struct pes_entry e;
		do {
			e = array_get_at(lp->p.pes, last_i++);
		} while(!pes_entry_is_received(e));

		const struct lp_msg *const msg = pes_entry_msg_received(e);
		global_config.dispatcher(msg->dest, msg->dest_t, msg->m_type, msg->pl, msg->pl_size, state_p);
		stats_take(STATS_MSG_SILENT, 1);
	} while(last_i < past_i);

	silent_processing = false;
	stats_take(STATS_MSG_SILENT_TIME, timer_hr_value(t));
}

/**
 * @brief Send anti-messages
 * @param msg_processing the message processing data for the LP that has to send anti-messages
 * @param past_i the index in @a proc_p of the last validly processed message
 */
static inline void send_anti_messages(struct process_ctx *msg_processing, const array_count_t past_i)
{
	const array_count_t p_cnt = array_count(msg_processing->pes);
	array_count(msg_processing->pes) = past_i;
	for(array_count_t i = past_i; i < p_cnt; ++i) {
		struct pes_entry e = array_get_at(msg_processing->pes, i);

		while(!pes_entry_is_received(e)) {
			struct lp_msg *const msg = pes_entry_msg(e);
			if(pes_entry_is_sent_remote(e)) {
				const nid_t dest_nid = lid_to_nid(msg->dest);
				mpi_remote_anti_msg_send(msg, dest_nid);
				msg_allocator_free_at_gvt(msg);
			} else {
				const uint64_t f =
				    atomic_fetch_add_explicit(&msg->flags, MSG_FLAG_ANTI, memory_order_relaxed);
				if(f & MSG_FLAG_PROCESSED)
					msg_queue_insert(msg);
			}

			stats_take(STATS_MSG_ANTI, 1);
			e = array_get_at(msg_processing->pes, ++i);
		}

		struct lp_msg *msg = pes_entry_msg_received(e);
		const uint64_t f = atomic_fetch_add_explicit(&msg->flags, -MSG_FLAG_PROCESSED, memory_order_relaxed);
		if(!(f & MSG_FLAG_ANTI))
			msg_queue_insert_self(msg);
		stats_take(STATS_MSG_ROLLBACK, 1);
	}
}

/**
 * @brief Perform a rollback
 * @param lp the message processing data for the LP that has to rollback
 * @param past_i the index in @a proc_p of the last validly processed message
 */
static void do_rollback(struct lp_ctx *lp, const array_count_t past_i)
{
	const timer_uint t = timer_hr_new();
	send_anti_messages(&lp->p, past_i);
	const array_count_t last_i = model_allocator_checkpoint_restore(&lp->mm_state, past_i);
	stats_take(STATS_RECOVERY_TIME, timer_hr_value(t));
	stats_take(STATS_ROLLBACK, 1);
	silent_execution(lp, last_i, past_i);
	auto_ckpt_register_bad(&lp->auto_ckpt);
}

/**
 * @brief Find the last valid processed message with respect to a straggler message
 * @param msg_processing the message processing data for the LP
 * @param s_msg the straggler message
 * @return the index in @a proc_p of the last validly processed message
 */
static inline array_count_t match_straggler_msg(const struct process_ctx *msg_processing, const struct lp_msg *s_msg)
{
	for(array_count_t i = array_count(msg_processing->pes) - 1; i;) {
		const struct pes_entry e = array_get_at(msg_processing->pes, --i);
		if(pes_entry_is_received(e) && !msg_is_before(s_msg, pes_entry_msg_received(e)))
			return i + 1;
	}
	return 0;
}

/**
 * @brief Find the last valid processed message with respect to a received anti-message
 * @param msg_processing the message processing data for the LP
 * @param a_msg the anti-message
 * @return the index in @a proc_p of the last validly processed message
 */
static inline array_count_t match_anti_msg(const struct process_ctx *msg_processing, const struct lp_msg *a_msg)
{
	array_count_t i = array_count(msg_processing->pes) - 1;
	struct pes_entry e = array_get_at(msg_processing->pes, i);
	while(a_msg != (const struct lp_msg *)e.raw) { // this check relies on PES_ENTRY_RECEIVED being 0
		assert(i);
		e = array_get_at(msg_processing->pes, --i);
	}

	while(i) {
		if(pes_entry_is_received(array_get_at(msg_processing->pes, --i)))
			return i + 1;
	}
	return i;
}

/**
 * @brief Handle the reception of a remote anti-message
 * @param lp the message processing data for the LP that has to handle the anti-message
 * @param a_msg the remote anti-message
 */
static inline void handle_remote_anti_msg(struct lp_ctx *lp, struct lp_msg *a_msg)
{
	// Simplifies flags-based matching, also useful in the early remote anti-messages matching
	const uint64_t m_id = a_msg->raw_flags -= MSG_FLAG_ANTI;

	array_count_t i = array_count(lp->p.pes);
	if(unlikely(i == 0))
		goto insert_early_anti;

	const simtime_t m_time = a_msg->dest_t;
	for(struct pes_entry e = array_get_at(lp->p.pes, --i); likely(pes_entry_msg_received(e)->dest_t >= m_time);) {
		if(pes_entry_msg_received(e)->raw_flags == m_id) {
			while(i) {
				if(pes_entry_is_received(array_get_at(lp->p.pes, --i))) {
					i++;
					break;
				}
			}
			pes_entry_msg_received(e)->raw_flags |= MSG_FLAG_ANTI;
			do_rollback(lp, i);
			msg_allocator_free(pes_entry_msg_received(e));
			msg_allocator_free(a_msg);
			return;
		}

		do {
			if(unlikely(i == 0))
				goto insert_early_anti;
			e = array_get_at(lp->p.pes, --i);
		} while(!pes_entry_is_received(e));
	}

insert_early_anti:
	a_msg->next = lp->p.early_antis;
	lp->p.early_antis = a_msg;
}

/**
 * @brief Check if a remote message has already been invalidated by an early remote anti-message
 * @param msg_processing the message processing data of the current LP
 * @param msg the remote message to check
 * @return true if the message has been matched with an early remote anti-message, false otherwise
 *
 * Assumes msg_processing->early_antis is not NULL (so this should be checked before calling this)
 */
static inline bool check_early_anti_messages(struct process_ctx *msg_processing, struct lp_msg *msg)
{
	uint64_t m_id = msg->raw_flags;
	struct lp_msg **prev_p = &msg_processing->early_antis;
	struct lp_msg *a_msg = *prev_p;
	assert(a_msg);
	do {
		if(a_msg->raw_flags == m_id) {
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
static void handle_anti_msg(struct lp_ctx *lp, struct lp_msg *msg, const uint64_t last_flags)
{
	if(last_flags > (MSG_FLAG_ANTI | MSG_FLAG_PROCESSED)) {
		handle_remote_anti_msg(lp, msg);
		return;
	} else if(last_flags == (MSG_FLAG_ANTI | MSG_FLAG_PROCESSED)) {
		const array_count_t past_i = match_anti_msg(&lp->p, msg);
		do_rollback(lp, past_i);
		termination_on_lp_rollback(lp, msg->dest_t);
	}
	msg_allocator_free(msg);
}

/**
 * @brief Handle the reception of a straggler message
 * @param lp the processing context of the current LP
 * @param msg the received straggler message
 */
static void handle_straggler_msg(struct lp_ctx *lp, const struct lp_msg *msg)
{
	const array_count_t past_i = match_straggler_msg(&lp->p, msg);
	do_rollback(lp, past_i);
	termination_on_lp_rollback(lp, msg->dest_t);
}

/**
 * @brief Extract and process a message, if available
 *
 * This function encloses most of the actual parallel/distributed simulation logic.
 */
void process_msg(struct lp_msg *msg)
{
	gvt_on_msg_extraction(msg->dest_t);

	struct lp_ctx *lp = &lps[msg->dest];
	current_lp = lp;

	uint64_t flags = atomic_fetch_add_explicit(&msg->flags, MSG_FLAG_PROCESSED, memory_order_relaxed);
	if(unlikely(flags & MSG_FLAG_ANTI)) {
		handle_anti_msg(lp, msg, flags);
		lp->p.bound = unlikely(array_is_empty(lp->p.pes)) ? -1.0 : lp->p.bound;
		return;
	}

	if(unlikely(flags && lp->p.early_antis && check_early_anti_messages(&lp->p, msg)))
		return;

	if(unlikely(lp->p.bound >= msg->dest_t && msg_is_before(msg, pes_entry_msg_received(array_peek(lp->p.pes)))))
		handle_straggler_msg(lp, msg);

	if(unlikely(fossil_is_needed(lp))) {
		auto_ckpt_recompute(&lp->auto_ckpt, lp->mm_state.full_ckpt_size);
		fossil_lp_collect(lp);
	}

	common_msg_process(lp, msg);
	lp->p.bound = msg->dest_t;
	array_push(lp->p.pes, pes_entry_make(msg, PES_ENTRY_RECEIVED));

	auto_ckpt_register_good(&lp->auto_ckpt);
	if(auto_ckpt_is_needed(&lp->auto_ckpt))
		checkpoint_take(lp);

	termination_on_msg_process(lp, msg->dest_t);
}
