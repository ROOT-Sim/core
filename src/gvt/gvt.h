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

#include <stdalign.h>

extern void gvt_global_init(void);
extern simtime_t gvt_phase_run(void);
extern void gvt_on_msg_extraction(simtime_t msg_t);

union aligned_counter {
	alignas(CACHE_LINE_SIZE) atomic_uint c;
	alignas(CACHE_LINE_SIZE) unsigned raw;
};

_Static_assert(sizeof(union aligned_counter) == CACHE_LINE_SIZE,
		"unexpected aligned_counter size");
_Static_assert(offsetof(union aligned_counter, c) == 0,
		"unexpected aligned_counter alignment");
_Static_assert(offsetof(union aligned_counter, raw) == 0,
		"unexpected aligned_counter alignment");

extern __thread unsigned gvt_phase;
extern union aligned_counter remote_msg_sent[MSG_ID_PHASES][MAX_NODES];
extern atomic_int remote_msg_received[MSG_ID_PHASES];

extern void gvt_start_processing(void);
extern void gvt_on_done_ctrl_msg(void);

#define gvt_on_remote_msg_send(dest_nid)				\
__extension__({ atomic_fetch_add_explicit(&(remote_msg_sent[		\
	gvt_phase][dest_nid].c), 1U, memory_order_relaxed); })

#define gvt_on_remote_msg_receive(msg_phase)				\
__extension__({ atomic_fetch_add_explicit(remote_msg_received + 	\
	msg_phase, 1U, memory_order_relaxed); })
