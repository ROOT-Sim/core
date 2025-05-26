/**
 * @file mm/msg_allocator.c
 *
 * @brief Memory management functions for messages
 *
 * Memory management functions for messages
 *
 * SPDX-FileCopyrightText: 2008-2025 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <mm/msg_allocator.h>

#include <core/core.h>
#include <datatypes/array.h>
#include <gvt/gvt.h>

static _Thread_local dyn_array(struct lp_msg *) free_list = {0};
static _Thread_local dyn_array(struct lp_msg *) at_gvt_list = {0};

/**
 * @brief Initialize the message allocator thread-local data structures
 */
void msg_allocator_init(void)
{
	array_init(at_gvt_list);
	array_init(free_list);
}

/**
 * @brief Finalize the message allocator thread-local data structures
 */
void msg_allocator_fini(void)
{
	while(!array_is_empty(free_list))
		mm_free(array_pop(free_list));
	array_fini(free_list);

	while(!array_is_empty(at_gvt_list))
		mm_free(array_pop(at_gvt_list));
	array_fini(at_gvt_list);
}

/**
 * @brief Allocate a new message with given payload size
 * @param payload_size the size in bytes of the requested message payload
 * @return a new message with at least the requested amount of payload space
 *
 * Since this module relies on the member lp_msg.pl_size (see @a msg_allocator_free()), it has writing responsibility
 * on it.
 */
struct lp_msg *msg_allocator_alloc(const unsigned payload_size)
{
	struct lp_msg *ret;
	if(unlikely(payload_size > MSG_PAYLOAD_BASE_SIZE)) {
		ret = mm_alloc(offsetof(struct lp_msg, extra_pl) + (payload_size - MSG_PAYLOAD_BASE_SIZE));
	} else if(unlikely(array_is_empty(free_list))) {
		ret = mm_alloc(sizeof(struct lp_msg));
	} else {
		ret = array_pop(free_list);
	}
	ret->pl_size = payload_size;
	return ret;
}

/**
 * @brief Free a message
 * @param msg a pointer to the message to release
 */
void msg_allocator_free(struct lp_msg *msg)
{
	if(likely(msg->pl_size <= MSG_PAYLOAD_BASE_SIZE))
		array_push(free_list, msg);
	else
		mm_free(msg);
}

/**
 * @brief Free a message after its destination time is committed
 * @param msg a pointer to the message to release
 */
void msg_allocator_free_at_gvt(struct lp_msg *msg)
{
	array_push(at_gvt_list, msg);
}

/**
 * @brief Free the committed messages after a new GVT has been computed
 * @param current_gvt the latest value of the GVT
 */
void msg_allocator_on_gvt(const simtime_t current_gvt)
{
	for(array_count_t i = array_count(at_gvt_list); i-- > 0;) {
		struct lp_msg *msg = array_get_at(at_gvt_list, i);
		if(msg->dest_t < current_gvt) {
			msg_allocator_free(msg);
			array_lazy_remove_at(at_gvt_list, i);
		}
	}
}

/**
 * @brief Allocate a new message and populate it
 * @param receiver the id of the LP which must receive this message
 * @param timestamp the logical time at which this message must be processed
 * @param event_type a field which can be used by the model to distinguish them
 * @param payload the payload to copy into the message
 * @param payload_size the size in bytes of the payload to copy into the message
 * @return a new populated message
 */
extern struct lp_msg *msg_allocator_pack(lp_id_t receiver, simtime_t timestamp, unsigned event_type,
    const void *payload, unsigned payload_size);
