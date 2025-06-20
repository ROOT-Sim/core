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

#include <assert.h>

extern void gvt_global_init(void);
extern simtime_t gvt_phase_run(void);
extern void gvt_on_msg_extraction(simtime_t msg_t);

extern __thread _Bool gvt_phase;
extern __thread uint32_t remote_msg_seq[2][MAX_NODES];
extern __thread uint32_t remote_msg_received[2];

extern void gvt_start_processing(void);
extern void gvt_on_done_ctrl_msg(void);
extern void gvt_msg_barrier(void);

/**
 * @brief Generate a new unique ID meant to be used for remote messages
 * @param dest_nid the node identifier of the node that should receive this remote message
 *
 * Used internally in this header, do not call outside if you don't know what you are doing!
 * IDs generated with this macro will be waited for during the GVT algorithm!
 */
#define gvt_remote_id_next(dest_nid) (\
	(gvt_phase << 2) | ((nid + 1) << 3) | (tid << (3 + MAX_NODES_BITS)) |\
    	((uint64_t)remote_msg_seq[gvt_phase][(dest_nid)]++ << (3 + MAX_NODES_BITS + MAX_THREADS_BITS)))

/**
 * @brief Register an outgoing remote message in the GVT subsystem
 * @param msg the remote message to register
 * @param dest_nid the destination node id of the message
 */
static inline void gvt_remote_msg_send(struct lp_msg *msg, nid_t dest_nid)
{
	msg->raw_flags = gvt_remote_id_next(dest_nid);
}

/**
 * @brief Register an outgoing remote anti-message in the GVT subsystem
 * @param msg the remote anti-message to register
 * @param dest_nid the destination node id of the anti-message
 */
static inline void gvt_remote_anti_msg_send(struct lp_msg *msg, nid_t dest_nid)
{
	++remote_msg_seq[gvt_phase][dest_nid];
	msg->raw_flags |= gvt_phase;
}

/**
 * @brief Register an incoming remote message in the GVT subsystem
 * @param msg the remote message to register
 */
static inline void gvt_remote_msg_receive(struct lp_msg *msg)
{
	++remote_msg_received[(msg->raw_flags >> 2U) & 1U];
	msg->raw_flags &= ~(uint64_t)3U;
}

/**
 * @brief Register an incoming remote anti-message in the GVT subsystem
 * @param msg the remote anti-message to register
 */
static inline void gvt_remote_anti_msg_receive(struct lp_msg *msg)
{
	++remote_msg_received[msg->raw_flags & 1U];
	msg->raw_flags |= MSG_FLAG_ANTI;
}

/**
 * @brief Assign a global ID to a message invisibly to the GVT subsystem
 * @param msg the message that need an assigned id
 */
static inline void gvt_msg_id_assign(struct lp_msg *msg)
{
	uint64_t flags = msg->raw_flags;
	assert(!(flags >> 2));
	gvt_remote_msg_send(msg, nid);
	++remote_msg_received[gvt_phase];
	msg->raw_flags |= flags;
}

/**
 * @brief Generate a global ID for a message invisibly to the GVT subsystem
 */
static inline uint64_t gvt_msg_id_generate(void)
{
	++remote_msg_received[gvt_phase];
	return gvt_remote_id_next(nid);
}
