/**
 * @file gvt/gvt.h
 *
 * @brief Global Virtual Time
 *
 * SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <core/core.h>
#include <lp/msg.h>

extern _Thread_local bool gvt_color;
extern _Thread_local uint64_t remote_msg_seq;
extern _Thread_local int64_t remote_msg_balance[2];

extern void gvt_global_init(void);
extern void gvt_on_msg_extraction(simtime_t msg_t);
extern void gvt_on_done_ctrl_msg(void);
extern void gvt_start_processing(void);
extern simtime_t gvt_run(void);
extern void gvt_handling_done(void);
extern void gvt_msg_barrier(void);

/**
 * @brief Generate a new unique ID meant to be used for remote messages
 *
 * Used internally in this header, do not call outside if you don't know what you are doing!
 * IDs generated with this macro will be waited for during the GVT algorithm!
 */
#define gvt_remote_id_next()                                                                                           \
	(((uint64_t)(nid + 1) << 2) | ((uint64_t)rid << (2 + MAX_NODES_BITS)) |                                        \
	    (remote_msg_seq++ << (2 + MAX_NODES_BITS + MAX_THREADS_BITS)))

/**
 * @brief Register an outgoing remote message in the GVT subsystem
 * @param msg the remote message to register
 * @param dest_nid the destination node id of the message
 */
static inline void gvt_remote_msg_send(struct lp_msg *msg)
{
	--remote_msg_balance[gvt_color];
	msg->raw_flags = (gvt_color << 1U) | gvt_remote_id_next();
}

/**
 * @brief Register an outgoing remote anti-message in the GVT subsystem
 * @param msg the remote anti-message to register
 * @param dest_nid the destination node id of the anti-message
 */
static inline void gvt_remote_anti_msg_send(struct lp_msg *msg)
{
	--remote_msg_balance[gvt_color];
	msg->raw_flags |= gvt_color;
}

/**
 * @brief Register an incoming remote message in the GVT subsystem
 * @param msg the remote message to register
 */
static inline void gvt_remote_msg_receive(struct lp_msg *msg)
{
	++remote_msg_balance[(msg->raw_flags >> 1U) & 1U];
	msg->raw_flags &= ~(uint64_t)(MSG_FLAG_PROCESSED | MSG_FLAG_ANTI);
}

/**
 * @brief Register an incoming remote anti-message in the GVT subsystem
 * @param msg the remote anti-message to register
 */
static inline void gvt_remote_anti_msg_receive(struct lp_msg *msg)
{
	++remote_msg_balance[msg->raw_flags & 1U];
	msg->raw_flags &= ~(uint64_t)MSG_FLAG_PROCESSED;
	msg->raw_flags |= MSG_FLAG_ANTI;
}
