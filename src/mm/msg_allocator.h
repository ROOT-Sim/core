/**
 * @file mm/msg_allocator.h
 *
 * @brief Memory management functions for messages
 *
 * Memory management functions for messages
 *
 * SPDX-FileCopyrightText: 2008-2023 HPDCS Group <rootsim@googlegroups.com>
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
