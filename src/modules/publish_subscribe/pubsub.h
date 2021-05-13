#pragma once

#include <stdint.h>
#include <lp/msg.h>
#include <lp/lp.h>
#include <lib/lib_internal.h>
#include <lib/lib.h>
#include <core/intrinsics.h>

// Size of additional data needed by the pubsub messages delivered to threads
#define size_of_pubsub_info	(sizeof(size_t) + sizeof(lp_msg*))

extern void pubsub_lib_lp_init(void);
extern void pubsub_lib_global_init(void);

extern void PublishNewEvent(simtime_t timestamp, unsigned event_type, const void *event_content, unsigned event_size);
extern void Subscribe(lp_id_t subscriber_id, lp_id_t publisher_id);
extern void SubscribeAndHandle(lp_id_t subscriber_id, lp_id_t publisher_id, void* data);

/*
 * Here the user handles the unpacking of a published message into a local event.
 * The user shall call ScheduleNewEvent to send the resulting event.
 * However the user shall invoke ScheduleNewEvent exactly once, and the
 * scheduled event shall be directed to the "me" LP.
 * Failure to do so will result in the simulation crashing/results being incorrect.
 */
extern void ProcessPublishedEvent(lp_id_t me, simtime_t msg_ts, unsigned int event, const void* msg_content, unsigned int size, const void* user_data);

extern void node_handle_published_message(lp_msg* msg);
extern void thread_handle_published_message(lp_msg* msg);
extern void node_handle_published_antimessage(lp_msg* msg);
extern void thread_handle_published_antimessage(lp_msg* msg);

extern void pubsub_msg_free(lp_msg* msg);

extern void pubsub_msg_queue_insert(lp_msg* msg);

extern lp_msg* msg_allocator_realloc(lp_msg* msg, unsigned payload_size);
inline lp_msg* msg_allocator_realloc(lp_msg* msg, unsigned payload_size)
{
	lp_msg *ret;
	if(payload_size > BASE_PAYLOAD_SIZE){
		ret = mm_realloc( msg,
			offsetof(lp_msg, extra_pl) +
			(payload_size - BASE_PAYLOAD_SIZE)
		);
		ret->pl_size = payload_size;
		return ret;
	}
	if(msg->pl_size <= BASE_PAYLOAD_SIZE){// && payload_size < BASE_PAYLOAD_SIZE){
		// No need to reallocate, won't get any smaller
		msg->pl_size = payload_size;
		return msg;
	}
	
	// msg->pl_size > BASE_PAYLOAD_SIZE && payload_size < BASE_PAYLOAD_SIZE
	// This will probably just truncate the memory area
	ret = mm_realloc(msg, sizeof(lp_msg));
	ret->pl_size = payload_size;
	return ret;
}

#define is_pubsub_msg(msg)						\
__extension__({								\
	msg->flags & MSG_FLAG_PUBSUB;					\
})
