/**
 * @file gvt/gvt.h
 *
 * @brief Global Virtual Time
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <core/core.h>
#include <lp/msg.h>

extern void gvt_global_init(void);
extern simtime_t gvt_phase_run(void);
extern void gvt_on_msg_process(simtime_t msg_t);

#ifdef ROOTSIM_MPI

extern __thread bool gvt_phase_green;
extern __thread unsigned remote_msg_sent[MAX_NODES];
extern atomic_int remote_msg_received[2];

extern void gvt_on_start_ctrl_msg(void);
extern void gvt_on_done_ctrl_msg(void);

#define gvt_on_remote_msg_send(dest_nid)				\
__extension__({ remote_msg_sent[dest_nid]++; })

#define gvt_on_remote_msg_receive(msg_phase)				\
__extension__({ atomic_fetch_add_explicit(remote_msg_received + 	\
	msg_phase, 1U, memory_order_relaxed); })

#define gvt_phase_get() __extension__({ gvt_phase_green;})
#endif
