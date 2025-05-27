/**
 * @file mm/msg_allocator.h
 *
 * @brief Memory management functions for messages
 *
 * Memory management functions for messages
 *
 * SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <lp/msg.h>

#include <memory.h>

extern void msg_allocator_init(void);
extern void msg_allocator_fini(void);

extern struct lp_msg *msg_allocator_alloc(unsigned payload_size);
extern void msg_allocator_free(struct lp_msg *msg);
extern void msg_allocator_free_at_gvt(struct lp_msg *msg);
extern void msg_allocator_on_gvt(simtime_t current_gvt);

/**
 * @brief Allocates and populates a new message.
 *
 * This function allocates a new message with the specified payload size and populates
 * its fields with the provided parameters. If a payload is provided, it is copied
 * into the message's payload buffer.
 *
 * @param receiver The ID of the LP that will receive this message.
 * @param timestamp The logical time at which this message must be processed.
 * @param event_type A field used by the model to distinguish event types.
 * @param payload A pointer to the payload data to copy into the message.
 * @param payload_size The size in bytes of the payload to copy into the message.
 * @return A pointer to the newly allocated and populated message.
 */
static inline struct lp_msg *msg_allocator_pack(lp_id_t receiver, simtime_t timestamp, unsigned event_type,
    const void *payload, unsigned payload_size)
{
	struct lp_msg *msg = msg_allocator_alloc(payload_size);

	msg->dest = receiver;
	msg->dest_t = timestamp;
	msg->m_type = event_type;

	if(likely(payload_size))
		memcpy(msg->pl, payload, payload_size);
	return msg;
}
