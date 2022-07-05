#pragma once

#include <stdint.h>
#include <lp/msg.h>
#include <lp/lp.h>
#include <lib/lib_internal.h>
#include <lib/lib.h>
#include <core/intrinsics.h>
#include <datatypes/bitmap.h>
#include <distributed/mpi.h>

#define is_fresh_pubsub_msg(msg) (lid_to_nid((msg)->dest) != nid)
#define PUBSUB_DUMP_MSGS false

extern void pubsub_module_lp_init(void);
extern void pubsub_module_lp_fini(void);
extern void pubsub_module_init(void);
extern void pubsub_module_fini(void);
extern void pubsub_module_global_init(void);
extern void pubsub_module_global_fini(void);
extern void pubsub_on_gvt(simtime_t current_gvt);
extern void print_pubsub_topology_to_file(void);
extern void log_pubsub_msgs_to_file(struct lp_msg **msg_array,
    array_count_t size);

/// Send an Event to all the subscribed LPs.
extern void PublishNewEvent(simtime_t timestamp, unsigned event_type,
    const void *event_content, unsigned event_size);
/// Make LP subscriber_id receive a copy of each message published from LP
/// publisher_id.
extern void Subscribe(lp_id_t subscriber_id, lp_id_t publisher_id);
/// Make LP subscriber_id receive a copy of each message published from LP
/// publisher_id and unpack it with custom data.
extern void SubscribeAndHandle(lp_id_t subscriber_id, lp_id_t publisher_id,
    void *data);

/*
 * Here the user handles the unpacking of a published message into a local
 * event. The user shall call PubsubDeliver to deliver the resulting event.
 */
extern void ProcessPublishedEvent(lp_id_t me, simtime_t msg_ts,
    unsigned int event, const void *msg_content, unsigned int size,
    const void *user_data);
/* This function is called by the user from within ProcessPublishedEvent,
 * to provide the results of the unpacking to the module */
extern void PubsubDeliver(simtime_t timestamp, unsigned event_type,
    const void *event_content, unsigned event_size);

extern void pub_node_handle_published_message(struct lp_msg *msg);
extern void sub_node_handle_published_message(struct lp_msg *msg);
extern void thread_handle_published_message(struct lp_msg *msg);
extern void pub_node_handle_published_antimessage(struct lp_msg *msg);
extern void sub_node_handle_published_antimessage(struct lp_msg *msg);
extern void thread_handle_published_antimessage(struct lp_msg *anti_msg);

extern void pubsub_msg_free(struct lp_msg *msg);
extern void pubsub_thread_msg_free(struct lp_msg *msg);
extern void pubsub_thread_msg_free_in_fini(struct lp_msg *msg);
extern void pubsub_msg_queue_insert(struct lp_msg *msg);

#define is_pubsub_msg(msg) __extension__({ (msg)->raw_flags &MSG_FLAG_PUBSUB; })
