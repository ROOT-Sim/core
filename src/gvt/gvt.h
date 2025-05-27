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

extern void gvt_global_init(void);
extern simtime_t gvt_phase_run(void);
extern void gvt_on_msg_extraction(simtime_t msg_t);

extern _Thread_local _Bool gvt_phase;
extern _Thread_local uint32_t remote_msg_seq[2][MAX_NODES];
extern _Thread_local uint32_t remote_msg_received[2];

extern void gvt_start_processing(void);
extern void gvt_on_done_ctrl_msg(void);
extern void gvt_msg_drain(void);

/**
 * Registers an outgoing remote message in the GVT subsystem
 * @param msg the remote message to register
 * @param dest_nid the destination node id of the message
 */
static inline void gvt_remote_msg_send(struct lp_msg *msg, nid_t dest_nid)
{
	msg->m_seq = (remote_msg_seq[gvt_phase][dest_nid]++ << 1) | gvt_phase;
	msg->raw_flags = (nid << (MAX_THREADS_EXP + 2)) | ((rid + 1) << 2) | gvt_phase;
}

/**
 * Registers an outgoing remote anti-message in the GVT subsystem
 * @param msg the remote anti-message to register
 * @param dest_nid the destination node id of the anti-message
 */
static inline void gvt_remote_anti_msg_send(struct lp_msg *msg, nid_t dest_nid)
{
	++remote_msg_seq[gvt_phase][dest_nid];
	msg->raw_flags |= gvt_phase << 1U;
}

/**
 * Registers an incoming remote message in the GVT subsystem
 * @param msg the remote message to register
 */
static inline void gvt_remote_msg_receive(struct lp_msg *msg)
{
	++remote_msg_received[msg->raw_flags & 1U];
	msg->raw_flags &= ~((uint32_t)3U);
}

/**
 * Registers an incoming remote anti-message in the GVT subsystem
 * @param msg the remote anti-message to register
 */
static inline void gvt_remote_anti_msg_receive(struct lp_msg *msg)
{
	++remote_msg_received[(msg->raw_flags >> 1U) & 1U];
	msg->raw_flags &= ~((uint32_t)3U);
	msg->raw_flags |= MSG_FLAG_ANTI;
}
