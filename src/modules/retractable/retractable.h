#pragma once

#include <stdint.h>
#include <lp/msg.h>
#include <lp/lp.h>
#include <lib/lib_internal.h>
#include <lib/lib.h>

void retractable_lib_lp_init(void);

void msg_queue_insert_retractable();

//~ inline void retractable_msg_schedule(simtime_t timestamp, unsigned event_type);//, const void *payload, unsigned payload_size)
extern void retractable_msg_schedule(simtime_t timestamp, unsigned event_type);//, const void *payload, unsigned payload_size)

inline bool is_retractable(struct lp_msg* msg){
	return msg->flags & MSG_FLAG_RETRACTABLE;
}
extern bool is_retractable(struct lp_msg* msg);

inline bool is_retractable_and_valid(struct lp_msg* msg);
extern bool is_retractable_and_valid(struct lp_msg* msg);

inline bool is_valid_retractable(struct lp_msg* msg);
extern bool is_valid_retractable(struct lp_msg* msg);


extern bool is_retractable_dummy(struct lp_msg* msg);
inline bool is_retractable_dummy(struct lp_msg* msg){
	return msg->flags & (MSG_FLAG_RETRACTABLE & MSG_FLAG_PROCESSED);
}

void retractable_rollback_handle();

inline void ScheduleRetractableEvent(simtime_t timestamp, unsigned event_type){		
	retractable_msg_schedule(timestamp, event_type);
}

extern void DescheduleRetractableEvent();
inline void DescheduleRetractableEvent();
