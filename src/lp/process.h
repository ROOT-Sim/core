/**
 * @file lp/process.h
 *
 * @brief LP state management functions
 *
 * LP state management functions
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <datatypes/array.h>
#include <lp/msg.h>

/// The message processing data produced by the LP
struct process_ctx {
	/// The messages processed in the past by the owner LP
	dyn_array(struct lp_msg *) p_msgs;
	/// The list of remote anti-messages delivered before their original counterpart
	/** Hopefully this is 99.9% of the time empty */
	struct lp_msg *early_antis;
	/// The current logical time at which this LP is
	/** This is lazily updated and not always accurate; it's sufficient for faster straggler detection */
	simtime_t bound;
};

#define is_msg_sent(msg_p) (((uintptr_t)(msg_p)) & 3U)
#define is_msg_remote(msg_p) (((uintptr_t)(msg_p)) & 2U)
#define is_msg_local_sent(msg_p) (((uintptr_t)(msg_p)) & 1U)
#define is_msg_past(msg_p) (!(((uintptr_t)(msg_p)) & 3U))
#define unmark_msg(msg_p) ((struct lp_msg *)(((uintptr_t)(msg_p)) & (UINTPTR_MAX - 3)))

struct lp_ctx; // forward declaration

extern void process_lp_init(struct lp_ctx *lp);
extern void process_lp_fini(struct lp_ctx *lp);

extern void warp_process_msg(void);
extern void racer_process_msg(void);
extern simtime_t racer_last(void);

