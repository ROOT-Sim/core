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

#include <datatypes/msg_queue.h>
#include <distributed/mpi.h>
#include <gvt/gvt.h>
#include <log/stats.h>
#include <lp/lp.h>
#include <mm/msg_allocator.h>
#include <serial/serial.h>

static _Thread_local bool silent_processing = false;
static _Thread_local dyn_array(struct lp_msg *) early_antis;

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
	msg->send_t = array_peek(proc_p->past_msgs)->dest_t;
#endif

	nid_t dest_nid = lid_to_nid(receiver);
	if (dest_nid != nid) {
		mpi_remote_msg_send(msg, dest_nid);
		msg_allocator_free_at_gvt(msg);
	} else {
		atomic_store_explicit(&msg->flags, 0U, memory_order_relaxed);
		msg_queue_insert(msg);
	}
	array_push(proc_p->sent_msgs, msg);
}

void process_global_init(void)
{
	serial_model_init();
}

void process_global_fini(void)
{
	ProcessEvent(0, 0, MODEL_FINI, NULL, 0, NULL);
}

void process_init(void)
{
	array_init(early_antis);
}

void process_fini(void)
{
	array_fini(early_antis);
}

/**
 * @brief Initializes the processing module in the current LP
 */
void process_lp_init(void)
{
	struct lp_ctx *this_lp = current_lp;
	struct process_data *proc_p = &current_lp->p;

	array_init(proc_p->past_msgs);
	array_init(proc_p->sent_msgs);

	struct lp_msg *msg = msg_allocator_pack(this_lp - lps, 0, LP_INIT,
			NULL, 0U);

	array_push(proc_p->past_msgs, msg);
	array_push(proc_p->sent_msgs, NULL);
	ProcessEvent_pr(this_lp - lps, 0, LP_INIT, NULL, 0, NULL);

	model_allocator_checkpoint_next_force_full();
	model_allocator_checkpoint_take(0);
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

	array_fini(proc_p->sent_msgs);

	for (array_count_t i = 0; i < array_count(proc_p->past_msgs); ++i) {
		struct lp_msg *msg = array_get_at(proc_p->past_msgs, i);
		uint32_t flags = atomic_load_explicit(&msg->flags,
				memory_order_relaxed);
		if (!(flags & MSG_FLAG_ANTI))
			msg_allocator_free(msg);
	}
	array_fini(proc_p->past_msgs);
}

static inline void silent_execution(struct process_data *proc_p,
		array_count_t last_i, array_count_t past_i)
{
	silent_processing = true;

	void *state_p = current_lp->lib_ctx_p->state_s;
	for (array_count_t k = last_i + 1; k < past_i; ++k) {
		const struct lp_msg *msg = array_get_at(proc_p->past_msgs, k);
		stats_time_start(STATS_MSG_SILENT);
		ProcessEvent_pr(
			msg->dest,
			msg->dest_t,
			msg->m_type,
			msg->pl,
			msg->pl_size,
			state_p
		);
		stats_time_take(STATS_MSG_SILENT);
	}

	silent_processing = false;
}

static inline void send_anti_messages(struct process_data *proc_p,
		array_count_t past_i)
{
	array_count_t sent_i = array_count(proc_p->sent_msgs) - 1;
	array_count_t b = array_count(proc_p->past_msgs) - past_i;
	do {
		struct lp_msg *msg = array_get_at(proc_p->sent_msgs, sent_i);
		b -= msg == NULL;
		--sent_i;
	} while(b);

	for (array_count_t i = sent_i + 1; i < array_count(proc_p->sent_msgs);
			++i) {
		struct lp_msg *msg = array_get_at(proc_p->sent_msgs, i);
		if (!msg) {
			continue;
		}

        if (is_pubsub_msg(msg)){
            // a pubsub message sent from this LP
            node_handle_published_antimessage(msg);
            continue;
        }

		nid_t dest_nid = lid_to_nid(msg->dest);
		if (dest_nid != nid) {
			mpi_remote_anti_msg_send(msg, dest_nid);
		} else {
			int msg_status = atomic_fetch_add_explicit(&msg->flags,
				MSG_FLAG_ANTI, memory_order_relaxed);
			if (msg_status & MSG_FLAG_PROCESSED)
				msg_queue_insert(msg);
		}
	}
	array_count(proc_p->sent_msgs) = sent_i + 1;
}

static inline void reinsert_invalid_past_messages(struct process_data *proc_p,
		array_count_t past_i)
{
	for (array_count_t i = past_i; i < array_count(proc_p->past_msgs);
			++i) {
		struct lp_msg *msg = array_get_at(proc_p->past_msgs, i);
		int msg_status = atomic_fetch_add_explicit(&msg->flags,
			-MSG_FLAG_PROCESSED, memory_order_relaxed);
		if (!(msg_status & MSG_FLAG_ANTI)) {
			msg_queue_insert(msg);
		}
	}

	array_count(proc_p->past_msgs) = past_i;
}

static void do_rollback(struct process_data *proc_p, array_count_t past_i)
{
	stats_time_start(STATS_ROLLBACK);

	array_count_t last_i = model_allocator_checkpoint_restore(past_i - 1);
	silent_execution(proc_p, last_i, past_i);
	send_anti_messages(proc_p, past_i);
	reinsert_invalid_past_messages(proc_p, past_i);

	stats_time_take(STATS_ROLLBACK);
}

static inline array_count_t match_anti_msg(const struct process_data *proc_p,
		const struct lp_msg *a_msg)
{
	array_count_t past_i = array_count(proc_p->past_msgs) - 1;
	while (array_get_at(proc_p->past_msgs, past_i) != a_msg)
		--past_i;
	return past_i;
}

static inline array_count_t match_straggler_msg(
		const struct process_data *proc_p, const struct lp_msg *s_msg)
{
	array_count_t past_i = array_count(proc_p->past_msgs) - 2;
	while (!msg_is_before(array_get_at(proc_p->past_msgs, past_i), s_msg))
		--past_i;
	return past_i + 1;
}

static inline array_count_t match_remote_msg(const struct process_data *proc_p,
		const struct lp_msg *r_msg)
{
	uint32_t m_id = r_msg->raw_flags >> MSG_FLAGS_BITS, m_seq = r_msg->m_seq;
	array_count_t past_i = array_count(proc_p->past_msgs) - 1;
	while (past_i) {
		struct lp_msg *msg = array_get_at(proc_p->past_msgs, past_i);
		if (msg->raw_flags >> MSG_FLAGS_BITS == m_id && msg->m_seq == m_seq) {
			msg->raw_flags |= MSG_FLAG_ANTI;
			break;
		}
		--past_i;
	}

	return past_i;
}

static inline void handle_remote_anti_msg(struct process_data *proc_p,
		struct lp_msg *msg)
{
	array_count_t past_i = match_remote_msg(proc_p, msg);
	if (unlikely(!past_i)) {
		msg->raw_flags >>= MSG_FLAGS_BITS;
		array_push(early_antis, msg);
		return;
	}
	struct lp_msg *old_msg = array_get_at (proc_p->past_msgs, past_i);
	do_rollback(proc_p, past_i);
	termination_on_lp_rollback(msg->dest_t);
	msg_allocator_free(old_msg);
	msg_allocator_free(msg);
}

static inline bool check_early_anti_messages(struct lp_msg *msg)
{
	uint32_t m_id;
	if (likely(!array_count(early_antis) || !(m_id = msg->raw_flags >> MSG_FLAGS_BITS)))
		return false;
	uint32_t m_seq = msg->m_seq;
	if (!m_id)
		return false;
	for (array_count_t i = 0; i < array_count(early_antis); ++i) {
		struct lp_msg *a_msg = array_get_at(early_antis, i);
		if (a_msg->raw_flags == m_id && a_msg->m_seq == m_seq) {
			msg_allocator_free(msg);
			msg_allocator_free(a_msg);
			array_get_at(early_antis, i) = array_peek(early_antis);
			--array_count(early_antis);
			return true;
		}
	}
	return false;
}

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

	struct lp_ctx *this_lp = &lps[msg->dest];
	struct process_data *proc_p = &this_lp->p;
	current_lp = this_lp;

	gvt_on_msg_extraction(msg->dest_t);

	uint32_t flags = atomic_fetch_add_explicit(&msg->flags,
			MSG_FLAG_PROCESSED, memory_order_relaxed);

	if (unlikely(flags & MSG_FLAG_ANTI)) {
		if (flags > (MSG_FLAG_ANTI | MSG_FLAG_PROCESSED)) {
			handle_remote_anti_msg(proc_p, msg);
			return;
		}
		if (flags == (MSG_FLAG_ANTI | MSG_FLAG_PROCESSED)) {
			array_count_t past_i = match_anti_msg(proc_p, msg);
			do_rollback(proc_p, past_i);
			termination_on_lp_rollback(msg->dest_t);
		}
		msg_allocator_free(msg);
		return;
	}

	if (unlikely(check_early_anti_messages(msg)))
		return;

	if (unlikely(!msg_is_before(array_peek(proc_p->past_msgs), msg))) {
		array_count_t past_i = match_straggler_msg(proc_p, msg);
		do_rollback(proc_p, past_i);
		termination_on_lp_rollback(msg->dest_t);
	}

	array_push(proc_p->sent_msgs, NULL);
	array_push(proc_p->past_msgs, msg);

	stats_time_start(STATS_MSG_PROCESSED);
	ProcessEvent_pr(
		msg->dest,
		msg->dest_t,
		msg->m_type,
		msg->pl,
		msg->pl_size,
		this_lp->lib_ctx_p->state_s
	);
	stats_time_take(STATS_MSG_PROCESSED);

	model_allocator_checkpoint_take(array_count(proc_p->past_msgs) - 1);
	termination_on_msg_process(msg->dest_t);
}
