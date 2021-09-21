/**
 * @file lp/process.h
 *
 * @brief LP state management functions
 *
 * LP state management functions
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <datatypes/array.h>
#include <lp/msg.h>

/// The message processing data produced by the LP
struct process_data {
	/// The messages processed in the past by the owner LP
	dyn_array(struct lp_msg *) p_msgs;
	simtime_t last_t;
	unsigned ckpt_rem_msgs;
};

#define is_msg_sent(msg_p) (((uintptr_t)(msg_p)) & 1U)
#define is_msg_past(msg_p) (!is_msg_sent(msg_p))

extern void process_global_init(void);
extern void process_global_fini(void);

extern void process_init(void);
extern void process_fini(void);

extern void process_lp_init(void);
extern void process_lp_deinit(void);
extern void process_lp_fini(void);

extern void process_msg(void);
extern void process_ckpt_rate_set(unsigned rate);
