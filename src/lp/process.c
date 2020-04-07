#include <lp/process.h>

#include <lp/lp.h>
#include <mm/msg_allocator.h>

void ScheduleNewEvent(unsigned receiver, simtime_t timestamp,
	unsigned event_type, const void *payload, unsigned payload_size)
{
	lp_struct *this_lp = current_lp;
	if(this_lp->state == LP_STATE_SILENT)
		return;

	lp_msg *msg = msg_allocator_pack(receiver, timestamp, event_type,
		payload, payload_size);

	msg_queue_insert(msg);
	array_push(this_lp->p.sent_msgs, msg);
}

void process_lp_init(void)
{
	lp_struct *this_lp = current_lp;
	struct process_data *proc_p = &current_lp->p;

	array_init(proc_p->logs);
	array_init(proc_p->past_msgs);
	array_init(proc_p->sent_msgs);

	lp_msg *this_msg = msg_allocator_pack(
		nid_lid_to_lp_id(nid, this_lp - lps), 0, INIT, NULL, 0U);
	current_msg = this_msg;

	array_push(proc_p->sent_msgs, NULL);
	ProcessEvent(
		this_msg->dest,
		this_msg->dest_t,
		*((unsigned *) this_msg->pl),
		&this_msg->pl[sizeof(unsigned)],
		this_msg->pl_size - sizeof(unsigned),
		this_lp->lsm.state_s
	);

	struct log_entry entry = {
		.i_past_msg = 0,
		.chkp = model_checkpoint_take()
	};
	array_push(proc_p->logs, entry);

	array_push(proc_p->past_msgs, this_msg);
}

void process_lp_fini(void)
{
	struct process_data *proc_p = &current_lp->p;
	array_fini(proc_p->sent_msgs);
	array_fini(proc_p->past_msgs);
	array_fini(proc_p->logs);
}

static void restore_checkpoint(array_count_t past_i)
{
	struct process_data *proc_p = &current_lp->p;
	// free inconsistent checkpoints
	while(past_i < array_peek(proc_p->logs).i_past_msg){
		model_checkpoint_free(array_peek(proc_p->logs).chkp);
		// TODO: don't do the actual free during msg processing
		array_pop(proc_p->logs);
	}
	// restore the latest valid checkpoint
	model_checkpoint_restore(array_peek(proc_p->logs).chkp);
}

static void silent_execution(array_count_t past_i)
{
	lp_struct *this_lp = current_lp;
	struct process_data *proc_p = &this_lp->p;
	enum lp_state old_state = this_lp->state;
	current_lp->state = LP_STATE_SILENT;

	for(
		array_count_t k = array_peek(proc_p->logs).i_past_msg + 1;
		k <= past_i;
		++k
	){
		const lp_msg *this_msg = array_get_at(proc_p->past_msgs, k);
		ProcessEvent(
			this_msg->dest,
			this_msg->dest_t,
			*((unsigned *) this_msg->pl),
			&this_msg->pl[sizeof(unsigned)],
			this_msg->pl_size - sizeof(unsigned),
			this_lp->lsm.state_s
		);
	}

	this_lp->state = old_state;
}

static void send_anti_messages(array_count_t past_i)
{
	struct process_data *proc_p = &current_lp->p;
	array_count_t sent_i = array_count(proc_p->sent_msgs) - 1;
	for(
		array_count_t b = array_count(proc_p->past_msgs) - 1 - past_i;
		b;
		--sent_i
	){
		lp_msg *msg = array_get_at(proc_p->sent_msgs, sent_i);
		b -= msg == NULL;
	}

	for (array_count_t i = sent_i; i < array_count(proc_p->sent_msgs); ++i){
		lp_msg *msg = array_get_at(proc_p->sent_msgs, i);
		if(!msg)
			continue;

		msg_set_anti(msg);
		msg_queue_insert(msg);
	}

	array_count(proc_p->sent_msgs) = sent_i + 1;
}

static void reinsert_invalid_past_messages(array_count_t past_i)
{
	struct process_data *proc_p = &current_lp->p;
	for (array_count_t i = past_i; i < array_count(proc_p->past_msgs); ++i){
		lp_msg *msg = array_get_at(proc_p->past_msgs, i);
		msg_queue_insert(msg);
	}

	array_count(proc_p->past_msgs) = past_i + 1;
}

static unsigned handle_rollback(void)
{
	lp_msg *this_msg = current_msg;
	struct process_data *proc_p = &current_lp->p;

	array_count_t past_i = array_count(proc_p->past_msgs) - 2;
	while (msg_is_before(
		this_msg, array_get_at(proc_p->past_msgs, past_i))){
		--past_i;
	}

	restore_checkpoint(past_i);
	silent_execution(past_i);
	send_anti_messages(past_i);
	reinsert_invalid_past_messages(past_i);
}

void process_msg(void)
{
	lp_struct *this_lp = current_lp;
	lp_msg *this_msg = current_msg;
	struct process_data *proc_p = &this_lp->p;

	if(unlikely(msg_is_before(this_msg, array_peek(proc_p->past_msgs))))
		handle_rollback();

	array_push(proc_p->sent_msgs, NULL);
	ProcessEvent(
		this_msg->dest,
		this_msg->dest_t,
		*((unsigned *) this_msg->pl),
		&this_msg->pl[sizeof(unsigned)],
		this_msg->pl_size - sizeof(unsigned),
		this_lp->lsm.state_s
	);

	if (array_peek(proc_p->logs).i_past_msg + 20 <
		array_count(proc_p->past_msgs)) {
		struct log_entry entry = {
			.i_past_msg = array_count(proc_p->past_msgs),
			.chkp = model_checkpoint_take()
		};
		array_push(proc_p->logs, entry);
	}

	array_push(proc_p->past_msgs, this_msg);
}
