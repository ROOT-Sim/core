#pragma once

#include <stdint.h>
#include <lp/msg.h>
#include <lp/lp.h>
#include <lib/lib_internal.h>
#include <lib/lib.h>
#include <core/intrinsics.h>
#include <datatypes/bitmap.h>

#ifdef ROOTSIM_MPI
#include <distributed/mpi.h>
#endif

// Size of additional data needed by pubsub messages published locally
#define size_of_pubsub_info	(sizeof(size_t) + sizeof(struct lp_msg*))

extern void pubsub_module_lp_init(void);
extern void pubsub_module_global_init(void);

/// Send an Event to all the subscribed LPs.
extern void PublishNewEvent(simtime_t timestamp, unsigned event_type, const void *event_content, unsigned event_size);
/// Make LP subscriber_id receive a copy of each message published from LP publisher_id.
extern void Subscribe(lp_id_t subscriber_id, lp_id_t publisher_id);
/// Make LP subscriber_id receive a copy of each message published from LP publisher_id and unpack it with custom data.
extern void SubscribeAndHandle(lp_id_t subscriber_id, lp_id_t publisher_id, void* data);

/*
 * Here the user handles the unpacking of a published message into a local event.
 * The user shall call ScheduleNewEvent to send the resulting event.
 * However the user shall invoke ScheduleNewEvent exactly once, and the
 * scheduled event shall be directed to the "me" LP.
 * Failure to do so will result in the simulation crashing/results being incorrect.
 */
extern void ProcessPublishedEvent(lp_id_t me, simtime_t msg_ts, unsigned int event, const void* msg_content, unsigned int size, const void* user_data);

extern void pub_node_handle_published_message(struct lp_msg* msg);
extern void sub_node_handle_published_message(struct lp_msg* msg);
extern void thread_handle_published_message(struct lp_msg* msg);
extern void pub_node_handle_published_antimessage(struct lp_msg* msg);
extern void sub_node_handle_published_antimessage(struct lp_msg* msg);
extern void thread_handle_published_antimessage(struct lp_msg* anti_msg);

extern void pubsub_msg_free(struct lp_msg* msg);

extern void pubsub_msg_queue_insert(struct lp_msg* msg);

#define is_pubsub_msg(msg)						\
__extension__({								\
	(msg)->flags & MSG_FLAG_PUBSUB;					\
})
