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
	dyn_array(struct lp_msg *) past_msgs;
	/// The messages sent in the past by the owner LP
	/** During a single message execution a LP may send several messages,
	 *  therefore a NULL entry per message processed is inserted in the
	 *  array in order to distinguish the originator message */
	dyn_array(struct lp_msg *) sent_msgs;
};

extern void process_global_init(void);
extern void process_global_fini(void);

extern void process_init(void);
extern void process_fini(void);

extern void process_lp_init(void);
extern void process_lp_deinit(void);
extern void process_lp_fini(void);

extern void process_msg(void);
extern void process_msg_sent(struct lp_msg *msg);
