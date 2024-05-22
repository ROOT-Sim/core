/**
 * @file mm/msg_allocator.c
 *
 * @brief Memory management functions for messages
 *
 * Memory management functions for messages
 *
 * SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <mm/msg_allocator.h>

#include <datatypes/array.h>
#include <gvt/gvt.h>

#define MSG_ARRAY_FREE(msg_array)                                                                                      \
	__extension__({                                                                                                \
		for(array_count_t i = array_count(msg_array); i--;)                                                    \
			mm_free(array_get_at(msg_array, i));                                                           \
		array_fini(msg_array);                                                                                 \
	})

static __thread array_declare(struct lp_msg *) free_list;
static __thread array_declare(struct lp_msg *) free_at_gvt[2];
static __thread array_declare(struct lp_msg *) free_at_gvt_large[2];

/**
 * @brief Initialize the message allocator thread-local data structures
 */
void msg_allocator_init(void)
{
	array_init_explicit(free_at_gvt_large[0], 4U);
	array_init_explicit(free_at_gvt_large[1], 4U);
	array_init(free_at_gvt[0]);
	array_init(free_at_gvt[1]);
	array_init(free_list);
}

/**
 * @brief Finalize the message allocator thread-local data structures
 */
void msg_allocator_fini(void)
{
	MSG_ARRAY_FREE(free_list);
	MSG_ARRAY_FREE(free_at_gvt[1]);
	MSG_ARRAY_FREE(free_at_gvt[0]);
	MSG_ARRAY_FREE(free_at_gvt_large[1]);
	MSG_ARRAY_FREE(free_at_gvt_large[0]);
}

/**
 * @brief Allocate a new message with given payload size
 * @param payload_size the size in bytes of the requested message payload
 * @return a new message with at least the requested amount of payload space
 *
 * Since this module relies on the member lp_msg.pl_size (see @a msg_allocator_free()), it has writing responsibility
 * on it.
 */
struct lp_msg *msg_allocator_alloc(unsigned payload_size)
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
	if(likely(msg->pl_size <= MSG_PAYLOAD_BASE_SIZE))
		array_push(free_at_gvt[gvt_phase], msg);
	else
		array_push(free_at_gvt_large[gvt_phase], msg);
}

/**
 * @brief Free the committed messages after a new GVT has been computed
 * @param current_gvt the latest value of the GVT
 */
void msg_allocator_on_gvt(void)
{
	bool phase = !gvt_phase;
	for(array_count_t i = array_count(free_at_gvt_large[phase]); i--;)
		mm_free(array_get_at(free_at_gvt_large[phase], i));
	array_clear(free_at_gvt_large[phase]);

	array_extend(free_list, free_at_gvt[phase]);
	array_clear(free_at_gvt[phase]);
}
