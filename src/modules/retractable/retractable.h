#pragma once

#include <stdint.h>
#include <lp/msg.h>
#include <lp/lp.h>
#include <lib/lib_internal.h>
#include <lib/lib.h>

void retractable_lib_lp_init(void);

void msg_queue_insert_retractable();

extern void retractable_msg_schedule(simtime_t timestamp, unsigned event_type);//, const void *payload, unsigned payload_size)

inline bool is_retractable(const struct lp_msg* msg){
	return msg->flags & MSG_FLAG_RETRACTABLE;
}

inline bool is_valid_retractable(const struct lp_msg* msg){
	return (msg->dest_t) == (current_lp->lib_ctx_p->r_ts);
}

inline bool is_retractable_and_valid(const struct lp_msg* msg){
	return is_retractable(msg) && is_valid_retractable(msg);
}



void retractable_rollback_handle();

inline void ScheduleRetractableEvent(simtime_t timestamp, unsigned event_type){
	retractable_msg_schedule(timestamp, event_type);
}

inline void ScheduleRetractableEvent_pr(simtime_t timestamp, unsigned event_type){
	ScheduleRetractableEvent(timestamp, event_type);
}

inline void DescheduleRetractableEvent(){
	// Just set the timestamp in LP memory to be invalid
	current_lp->lib_ctx_p->r_ts = -1.0;
}
