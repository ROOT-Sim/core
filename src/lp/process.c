/**
 * @file lp/process.c
 *
 * @brief LP state management functions
 *
 * LP state management functions
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <lp/process.h>

#include <arch/timer.h>
#include <core/init.h>
#include <datatypes/msg_queue.h>
#include <distributed/mpi.h>
#include <gvt/gvt.h>
#include <log/stats.h>
#include <lp/lp.h>
#include <mm/auto_ckpt.h>
#include <mm/msg_allocator.h>
#include <serial/serial.h>

__thread bool silent_processing = false;
static __thread dyn_array(struct lp_msg_remote_match) early_antis;

#define mark_msg_remote(msg_p) ((struct lp_msg *)(((uintptr_t)(msg_p)) | 2U))
#define mark_msg_sent(msg_p) ((struct lp_msg *)(((uintptr_t)(msg_p)) | 1U))
#define unmark_msg_remote(msg_p) ((struct lp_msg *)(((uintptr_t)(msg_p)) - 2U))
#define unmark_msg_sent(msg_p) ((struct lp_msg *)(((uintptr_t)(msg_p)) - 1U))

void ScheduleNewEvent_pr(lp_id_t receiver, simtime_t timestamp,
		unsigned event_type, const void *payload, unsigned payload_size)
{
	if (unlikely(silent_processing))
		return;

	struct process_data *proc_p = &current_lp->p;
	struct lp_msg *msg = msg_allocator_pack(receiver, timestamp, event_type,
		payload, payload_size);

#if LOG_LEVEL <= LOG_DEBUG
	msg->send = current_lp - lps;
	msg->send_t = proc_p->last_t;
#endif

	nid_t dest_nid = lid_to_nid(receiver);
	if (dest_nid != nid) {
		mpi_remote_msg_send(msg, dest_nid);
		array_push(proc_p->p_msgs, mark_msg_remote(msg));
	} else {
		atomic_store_explicit(&msg->flags, 0U, memory_order_relaxed);
		msg_queue_insert(msg);
		array_push(proc_p->p_msgs, mark_msg_sent(msg));
	}
}


/**
 * @brief Initialize the global state of the simulation
 *
 * Setup the global state of the model by calling its MODEL_INIT handler
 */
void process_global_init(void)
{
	serial_model_init();
}

/**
 * @brief Finalize the global state of the simulation
 *
 * Finalize the global state of the model by calling its MODEL_FINI handler
 */
void process_global_fini(void)
{
	ProcessEvent(0, 0, MODEL_FINI, NULL, 0, NULL);
}

/**
 * @brief Initialize the thread-local processing data structures
 */
void process_init(void)
{
	array_init(early_antis);
}

/**
 * @brief Finalize the thread-local processing data structures
 */
void process_fini(void)
{
	array_fini(early_antis);
}

static inline void checkpoint_take(struct process_data *proc_p)
{
	timer_uint t = timer_hr_new();
	model_allocator_checkpoint_take(array_count(proc_p->p_msgs));
	stats_take(STATS_CKPT, 1);
	stats_take(STATS_CKPT_TIME, timer_hr_value(t));
}

/**
 * @brief Initializes the processing module in the current LP
 */
void process_lp_init(void)
{
	struct lp_ctx *lp = current_lp;
	struct process_data *proc_p = &lp->p;

	array_init(proc_p->p_msgs);

	struct lp_msg *msg = msg_allocator_pack(lp - lps, 0, LP_INIT, NULL, 0U);
	msg->flags = 0;

	proc_p->last_t = 0;

	ProcessEvent_pr(lp - lps, 0, LP_INIT, NULL, 0, NULL);
	stats_take(STATS_MSG_PROCESSED, 1);

	array_push(proc_p->p_msgs, msg);
	model_allocator_checkpoint_next_force_full();
	checkpoint_take(proc_p);
}

/**
 * @brief Deinitializes the LP by calling the model's DEINIT handler
 */
void process_lp_deinit(void)
{
	struct lp_ctx *this_lp = current_lp;
	ProcessEvent_pr(this_lp - lps, 0, LP_FINI, NULL, 0,
			   this_lp->lib_ctx_p->state_s);
}

/**
 * @brief Finalizes the processing module in the current LP
 */
void process_lp_fini(void)
{
	struct process_data *proc_p = &current_lp->p;
	for (array_count_t i = 0; i < array_count(proc_p->p_msgs); ++i) {
		struct lp_msg *msg = array_get_at(proc_p->p_msgs, i);
		if (is_msg_local_sent(msg))
			continue;
		msg = unmark_msg(msg);
		uint32_t flags = atomic_load_explicit(&msg->flags,
				memory_order_relaxed);
		if (!(flags & MSG_FLAG_ANTI))
			msg_allocator_free(msg);
	}
	array_fini(proc_p->p_msgs);
}

static inline void silent_execution(const struct process_data *proc_p,
		array_count_t last_i, array_count_t past_i)
{
	if (unlikely(last_i >= past_i))
		return;

	timer_uint t = timer_hr_new();
	silent_processing = true;

	void *state_p = current_lp->lib_ctx_p->state_s;
	do {
		const struct lp_msg *msg = array_get_at(proc_p->p_msgs, last_i);
		while (is_msg_sent(msg)) {
			msg = array_get_at(proc_p->p_msgs, ++last_i);
		}

		ProcessEvent_pr(
			msg->dest,
			msg->dest_t,
			msg->m_type,
			msg->pl,
			msg->pl_size,
			state_p
		);
		stats_take(STATS_MSG_SILENT, 1);
	} while (++last_i < past_i);

	silent_processing = false;
	stats_take(STATS_MSG_SILENT_TIME, timer_hr_value(t));
}

static inline void send_anti_messages(struct process_data *proc_p,
		array_count_t past_i)
{
	array_count_t p_cnt = array_count(proc_p->p_msgs);
	for (array_count_t i = past_i; i < p_cnt; ++i) {
		struct lp_msg *msg = array_get_at(proc_p->p_msgs, i);

		while (is_msg_sent(msg)) {
#ifdef PUBSUB
			if (is_pubsub_msg(unmark_msg(msg))){
				// a pubsub message sent from this LP
				pub_node_handle_published_antimessage(unmark_msg(msg));
			} else
#endif
			if (is_msg_remote(msg)) {
				msg = unmark_msg_remote(msg);
				nid_t dest_nid = lid_to_nid(msg->dest);
				mpi_remote_anti_msg_send(msg, dest_nid);
			} else {
				msg = unmark_msg_sent(msg);
				int f = atomic_fetch_add_explicit(&msg->flags,
					MSG_FLAG_ANTI, memory_order_relaxed);
				if (f & MSG_FLAG_PROCESSED)
					msg_queue_insert(msg);
			}

			msg = array_get_at(proc_p->p_msgs, ++i);
		}

		int f = atomic_fetch_add_explicit(&msg->flags,
			-MSG_FLAG_PROCESSED, memory_order_relaxed);
		if (!(f & MSG_FLAG_ANTI)) {
			msg_queue_insert(msg);
			stats_take(STATS_MSG_ROLLBACK, 1);
		}
	}
	array_count(proc_p->p_msgs) = past_i;
}

static void do_rollback(struct process_data *proc_p, array_count_t past_i)
{
	send_anti_messages(proc_p, past_i);
	array_count_t last_i = model_allocator_checkpoint_restore(past_i);
	silent_execution(proc_p, last_i, past_i);
	stats_take(STATS_ROLLBACK, 1);
}

static inline array_count_t match_straggler_msg(
		const struct process_data *proc_p, const struct lp_msg *s_msg)
{
	array_count_t i = array_count(proc_p->p_msgs) - 1;
	const struct lp_msg *msg = array_get_at(proc_p->p_msgs, i);
	while (1) {
		if (msg_is_before(msg, s_msg))
			return i + 1;
		while (1) {
			if (!i)
				return 0;
			msg = array_get_at(proc_p->p_msgs, --i);
			if (is_msg_past(msg))
				break;
		}
	}
}

static inline array_count_t match_anti_msg(
		const struct process_data *proc_p, const struct lp_msg *a_msg)
{
	array_count_t past_i = 0;
	do {
		array_count_t j = past_i;
		const struct lp_msg *msg = array_get_at(proc_p->p_msgs, j);
		while (is_msg_sent(msg))
			msg = array_get_at(proc_p->p_msgs, ++j);

		if (msg == a_msg) {
			return past_i;
		}

		past_i = j + 1;
	} while(1);
}

static inline void handle_remote_anti_msg(struct process_data *proc_p,
		const struct lp_msg *msg)
{
	uint32_t m_id = msg->raw_flags >> MSG_FLAGS_BITS, m_seq = msg->m_seq;
	array_count_t p_cnt = array_count(proc_p->p_msgs), past_i = 0;
	while (likely(past_i < p_cnt)) {
		array_count_t j = past_i;
		struct lp_msg *amsg = array_get_at(proc_p->p_msgs, j);
		while (is_msg_sent(amsg))
			amsg = array_get_at(proc_p->p_msgs, ++j);

		if (amsg->raw_flags >> MSG_FLAGS_BITS == m_id && amsg->m_seq == m_seq) {
			// suppresses the re-insertion in the processing queue
			amsg->raw_flags |= MSG_FLAG_ANTI;
			do_rollback(proc_p, past_i);
			proc_p->last_t = past_i == 0 ? 0 :
				array_get_at(proc_p->p_msgs, --past_i)->dest_t;
			termination_on_lp_rollback(msg->dest_t);
			msg_allocator_free(amsg);
			return;
		}

		past_i = j + 1;
	}

	struct lp_msg_remote_match r = {
		.raw_flags = m_id,
		.m_seq = m_seq
	};
	array_push(early_antis, r);
}

static inline bool check_early_anti_messages(struct lp_msg *msg)
{
	uint32_t m_id;
	if (likely(!array_count(early_antis) || !(m_id = msg->raw_flags >> MSG_FLAGS_BITS)))
		return false;

	uint32_t m_seq = msg->m_seq;
	for (array_count_t i = array_count(early_antis); i > 0;) {
		struct lp_msg_remote_match *m = &array_get_at(early_antis, --i);
		if (unlikely(m->raw_flags == m_id && m->m_seq == m_seq)) {
			msg_allocator_free(msg);
			array_get_at(early_antis, i) = array_peek(early_antis);
			--array_count(early_antis);
			return true;
		}
	}
	return false;
}

/**
 * @brief extract and process a message, if available
 *
 * This function encloses most of the actual simulation logic.
 */
void process_msg(void)
{
	struct lp_msg *msg = msg_queue_extract();
	if (unlikely(!msg)) {
		current_lp = NULL;
		return;
	}

#ifdef PUBSUB
	if(is_pubsub_msg(msg)){
		// A pubsub message that needs to be unpacked
		if(unlikely(msg->flags & MSG_FLAG_ANTI)){
			thread_handle_published_antimessage(msg);
		} else {
			thread_handle_published_message(msg);
		}
		return;
	}
#endif

	gvt_on_msg_extraction(msg->dest_t);

	struct lp_ctx *this_lp = &lps[msg->dest];
	struct process_data *proc_p = &this_lp->p;
	current_lp = this_lp;

	uint32_t flags = atomic_fetch_add_explicit(&msg->flags,
			MSG_FLAG_PROCESSED, memory_order_relaxed);

	if (unlikely(flags & MSG_FLAG_ANTI)) {
		if (flags >> MSG_FLAGS_BITS) {
			handle_remote_anti_msg(proc_p, msg);
			auto_ckpt_register_bad(&this_lp->auto_ckpt);
		} else if (flags == (MSG_FLAG_ANTI | MSG_FLAG_PROCESSED)) {
			array_count_t past_i = match_anti_msg(proc_p, msg);
			do_rollback(proc_p, past_i);
			proc_p->last_t = past_i == 0 ? 0 :
				array_get_at(proc_p->p_msgs, --past_i)->dest_t;
			termination_on_lp_rollback(msg->dest_t);
			auto_ckpt_register_bad(&this_lp->auto_ckpt);
		}
		msg_allocator_free(msg);

		return;
	}

	if (unlikely(check_early_anti_messages(msg)))
		return;

	if (unlikely(proc_p->last_t > msg->dest_t)) {
		array_count_t past_i = match_straggler_msg(proc_p, msg);
		do_rollback(proc_p, past_i);
		termination_on_lp_rollback(msg->dest_t);
		auto_ckpt_register_bad(&this_lp->auto_ckpt);
	}

	proc_p->last_t = msg->dest_t;

	ProcessEvent_pr(
		msg->dest,
		msg->dest_t,
		msg->m_type,
		msg->pl,
		msg->pl_size,
		this_lp->lib_ctx_p->state_s
	);
	stats_take(STATS_MSG_PROCESSED, 1);
	array_push(proc_p->p_msgs, msg);

	auto_ckpt_register_good(&this_lp->auto_ckpt);
	if (auto_ckpt_is_needed(&this_lp->auto_ckpt))
		checkpoint_take(proc_p);

	termination_on_msg_process(msg->dest_t);
}

