#include <lib/retractable/retractable.h>

//~ extern _Thread_local bool silent_processing;

#define current_lid (current_lp-lps)

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
	struct lp_msg* msg = current_lp->r_msg;
	
	unsigned cur_rid = lid_to_rid(current_lid);
	
	//~ struct queue_t *this_q = &mqueue(cur_rid, cur_rid);
	struct msg_queue *this_q = mqueue(cur_rid, cur_rid);
	
	if(current_lp->lib_ctx_p->r_ts < 0) // The message is not to be scheduled
		return;
	
	// The message is to be scheduled
	
	if(msg==NULL){ // Need to create a new retractable msg
		msg = msg_allocator_pack(current_lid, current_lp->lib_ctx_p->r_ts,
			current_lp->lib_ctx_p->r_e_type, NULL, 0);
			
		msg->flags = MSG_FLAG_RETRACTABLE;
		current_lp->r_msg = msg;
		
		heap_insert(this_q->q, msg_is_before, msg);
		
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
		heap_priority_changed(this_q->q, msg, msg_is_before);
	} else {
		heap_insert(this_q->q, msg_is_before, msg);
	}
}

void retractable_rollback_handle(){
	msg_queue_insert_retractable();
	return;
}

inline void DescheduleRetractableEvent()
{
	// Just set the timestamp in LP memory to be invalid
	current_lp->lib_ctx_p->r_ts = -1.0;
}

inline void retractable_msg_schedule(simtime_t timestamp, unsigned event_type)//, const void *payload, unsigned payload_size)
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

inline bool is_retractable_and_valid(struct lp_msg* msg){
	//~ return (msg==(lps[msg->dest].r_msg)) && (msg->dest_t == current_lp->lsm_p->r_ts);
	return is_retractable(msg) && is_valid_retractable(msg);
}

inline bool is_valid_retractable(struct lp_msg* msg){
	return (msg->dest_t) == (current_lp->lib_ctx_p->r_ts);
}

//~ inline bool is_retractable(struct lp_msg* msg){
	//~ return msg==lps[msg->dest].r_msg;
//~ }

//~ inline bool is_retractable_dummy(struct lp_msg* msg){
	//~ // & with MSG_FLAG_PROCESSED is not needed because of when this check is carried out
	//~ return msg->flags & MSG_FLAG_RETRACTABLE;// & MSG_FLAG_PROCESSED
//~ }

#undef current_lp
