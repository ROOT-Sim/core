#include <modules/retractable/retractable.h>

//~ extern _Thread_local bool silent_processing;

#define current_lid (current_lp-lps)

extern bool is_retractable(const struct lp_msg* msg);
extern bool is_retractable_and_valid(const struct lp_msg* msg);
extern bool is_valid_retractable(const struct lp_msg* msg);
extern void DescheduleRetractableEvent();

void retractable_lib_lp_init(){
	
	current_lp->r_msg = NULL;
	
	current_lp->lib_ctx_p->r_ts = -1.0;
	current_lp->lib_ctx_p->r_e_type = -1;

	return;
}

/* This function populates the retractable message with the correct info
 * taken from the LP's lib state managed and then schedules it/moves it
 * to the right position in the input queue */
void msg_queue_insert_retractable()
{
	if(current_lp->lib_ctx_p->r_ts < 0) // The message is not to be scheduled
		return;
	
	// The message is to be scheduled
	struct lp_msg* msg = current_lp->r_msg;
	struct q_elem q_el;

	if(msg==NULL){ // Need to create a new retractable msg
		msg = msg_allocator_pack(current_lid, current_lp->lib_ctx_p->r_ts,
			current_lp->lib_ctx_p->r_e_type, NULL, 0);

		msg->flags = MSG_FLAG_RETRACTABLE;
		current_lp->r_msg = msg;

		q_el = (struct q_elem){.t = msg->dest_t, .m=msg};
		heap_insert(mqp.q, q_elem_is_before, q_el);

		return;
	}
	
	// Msg already exists

	// Is the message already in the incoming queue?
	// Second part of the check is not needed because of how messages are handled when extracted
	bool already_in_Q = (msg->dest_t >= 0);// && !(msg->flags & MSG_FLAG_PROCESSED);
	
	// Set the correct values for the message
	msg->dest_t = current_lp->lib_ctx_p->r_ts;
	msg->m_type = current_lp->lib_ctx_p->r_e_type;

	// Schedule the message
	if(already_in_Q){
		q_el = array_get_at(mqp.q, msg->pos);
		heap_priority_changed(mqp.q, q_el, q_elem_is_before);
	} else {
		q_el = (struct q_elem){.t=msg->dest_t, .m=msg};
		heap_insert(mqp.q, q_elem_is_before, q_el);
	}
}

void retractable_rollback_handle(){
	msg_queue_insert_retractable();
}

void retractable_msg_schedule(simtime_t timestamp, unsigned event_type)//, const void *payload, unsigned payload_size)
{

	current_lp->lib_ctx_p->r_ts = timestamp;
	current_lp->lib_ctx_p->r_e_type = event_type;
	
	/* When silent processing, we still want to update the values
	 * in lib_ctx, while not sending the msg */
	if(silent_processing){
		return;
	}

	// Move the message in the input queue!
	msg_queue_insert_retractable();

	return;
}

extern void ScheduleRetractableEvent(simtime_t timestamp, unsigned event_type);
extern void ScheduleRetractableEvent_pr(simtime_t timestamp, unsigned event_type);

#undef current_lid
