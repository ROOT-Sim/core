#include <lp/process.h>

#include <datatypes/msg_queue.h>
#include <distributed/mpi.h>
#include <lp/lp.h>
#include <mm/msg_allocator.h>

static __thread bool silent_processing = false;

void ScheduleNewEvent(lp_id_t receiver, simtime_t timestamp,
	unsigned event_type, const void *payload, unsigned payload_size)
{
	if(silent_processing)
		return;

	struct process_data *proc_p = &current_lp->p;
	lp_msg *msg = msg_allocator_pack(receiver, timestamp, event_type,
		payload, payload_size);

#ifdef NEUROME_MPI
	nid_t dest_nid = lid_to_nid(receiver);
	if (dest_nid != nid) {
		mpi_remote_msg_send(msg, dest_nid);
		msg_allocator_free_at_gvt(msg);
	} else {
#else
	{
#endif
		atomic_store_explicit(&msg->flags, 0U, memory_order_relaxed);
		msg_queue_insert(msg);
	}
	array_push(proc_p->sent_msgs, msg);
}

void process_lp_init(void)
{
	lp_struct *this_lp = current_lp;
	struct process_data *proc_p = &current_lp->p;

	array_init(proc_p->past_msgs);
	array_init(proc_p->sent_msgs);

	lp_msg *this_msg = msg_allocator_pack(this_lp - lps, 0, INIT, NULL, 0U);

	array_push(proc_p->sent_msgs, NULL);
	ProcessEvent(this_lp - lps, 0, INIT, NULL, 0, NULL);

	model_allocator_checkpoint_next_force_full();
	model_allocator_checkpoint_take(0);
	array_push(proc_p->past_msgs, this_msg);
}

void process_lp_deinit(void)
{
	lp_struct *this_lp = current_lp;
	ProcessEvent(this_lp - lps, 0, DEINIT, NULL, 0, this_lp->lsm_p->state_s);
}

void process_lp_fini(void)
{
	struct process_data *proc_p = &current_lp->p;

	array_fini(proc_p->sent_msgs);

	for (array_count_t i = 0; i < array_count(proc_p->past_msgs); ++i) {
		msg_allocator_free(array_get_at(proc_p->past_msgs, i));
	}
	array_fini(proc_p->past_msgs);
}

static inline void silent_execution(
	struct process_data *proc_p, array_count_t last_i, array_count_t past_i)
{
	silent_processing = true;

	void *state_p = current_lp->lsm_p->state_s;
	for (array_count_t k = last_i + 1; k <= past_i; ++k) {
		const lp_msg *this_msg = array_get_at(proc_p->past_msgs, k);
#ifdef NEUROME_INCREMENTAL
		ProcessEvent_instr(
#else
		ProcessEvent(
#endif
			this_msg->dest,
			this_msg->dest_t,
			this_msg->m_type,
			this_msg->pl,
			this_msg->pl_size,
			state_p
		);
	}

	silent_processing = false;
}

static inline void send_anti_messages(
	struct process_data *proc_p, array_count_t past_i)
{
	array_count_t sent_i = array_count(proc_p->sent_msgs) - 1;
	array_count_t b = array_count(proc_p->past_msgs) - 1 - past_i;
	do {
		lp_msg *msg = array_get_at(proc_p->sent_msgs, sent_i);
		b -= msg == NULL;
		--sent_i;
	} while(b);

	for (array_count_t i = sent_i + 1; i < array_count(proc_p->sent_msgs);
		++i) {
		lp_msg *msg = array_get_at(proc_p->sent_msgs, i);
		if(!msg)
			continue;

#ifdef NEUROME_MPI
		nid_t dest_nid = lid_to_nid(msg->dest);
		if (dest_nid != nid) {
			mpi_remote_anti_msg_send(msg, dest_nid);
		} else {
#else
		{
#endif
			int msg_status = atomic_fetch_add_explicit(&msg->flags,
				MSG_FLAG_ANTI, memory_order_relaxed);
			if(msg_status & MSG_FLAG_PROCESSED) {
				msg_queue_insert(msg);
			}
		}
	}
	array_count(proc_p->sent_msgs) = sent_i + 1;
}

static inline void reinsert_invalid_past_messages(
	struct process_data *proc_p, array_count_t past_i)
{
	for (array_count_t i = past_i + 1; i < array_count(proc_p->past_msgs);
		++i) {
		lp_msg *msg = array_get_at(proc_p->past_msgs, i);
		int msg_status = atomic_fetch_add_explicit(&msg->flags,
			-MSG_FLAG_PROCESSED, memory_order_relaxed);
		if(!(msg_status & MSG_FLAG_ANTI))
			msg_queue_insert(msg);
	}

	array_count(proc_p->past_msgs) = past_i + 1;
}

static void handle_rollback(struct process_data *proc_p, array_count_t past_i)
{
	array_count_t last_i = model_allocator_checkpoint_restore(past_i);
	silent_execution(proc_p, last_i, past_i);
	send_anti_messages(proc_p, past_i);
	reinsert_invalid_past_messages(proc_p, past_i);
}

static inline array_count_t match_anti_msg(
	const struct process_data *proc_p, const lp_msg *a_msg)
{
	array_count_t past_i = array_count(proc_p->past_msgs) - 1;
	while (array_get_at(proc_p->past_msgs, past_i) != a_msg) {
		--past_i;
	}
	return past_i - 1;
}

static inline array_count_t match_straggler_msg(
	const struct process_data *proc_p, const lp_msg *s_msg)
{
	array_count_t past_i = array_count(proc_p->past_msgs) - 2;
	while (!msg_is_before(array_get_at(proc_p->past_msgs, past_i), s_msg)) {
		--past_i;
	}
	return past_i;
}

void process_msg(void)
{
	lp_msg *this_msg = msg_queue_extract();
	if (unlikely(!this_msg)) {
		current_lp = NULL;
		return;
	}

	lp_struct *this_lp = &lps[this_msg->dest];
	struct process_data *proc_p = &this_lp->p;
	current_lp = this_lp;

	int msg_status = atomic_fetch_add_explicit(&this_msg->flags,
		MSG_FLAG_PROCESSED, memory_order_relaxed);
	if (unlikely(msg_status & MSG_FLAG_ANTI)) {
		if(msg_status == (MSG_FLAG_ANTI | MSG_FLAG_PROCESSED)){
			array_count_t past_i = match_anti_msg(proc_p, this_msg);
			handle_rollback(proc_p, past_i);
			termination_on_lp_rollback(this_msg->dest_t);
		}
		msg_allocator_free(this_msg);
		return;
	}

	if (unlikely(!msg_is_before(array_peek(proc_p->past_msgs), this_msg))){
		array_count_t past_i = match_straggler_msg(proc_p, this_msg);
		handle_rollback(proc_p, past_i);
		termination_on_lp_rollback(this_msg->dest_t);
	}

	array_push(proc_p->sent_msgs, NULL);
#ifdef NEUROME_INCREMENTAL
	ProcessEvent_instr(
#else
	ProcessEvent(
#endif
		this_msg->dest,
		this_msg->dest_t,
		this_msg->m_type,
		this_msg->pl,
		this_msg->pl_size,
		this_lp->lsm_p->state_s
	);

	model_allocator_checkpoint_take(array_count(proc_p->past_msgs));
	array_push(proc_p->past_msgs, this_msg);
	termination_on_msg_process(this_msg->dest_t);
	gvt_on_msg_process(this_msg->dest_t);
}
