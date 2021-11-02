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
};

#define is_msg_sent(msg_p) (((uintptr_t)(msg_p)) & 3U)
#define is_msg_remote(msg_p) (((uintptr_t)(msg_p)) & 2U)
#define is_msg_local_sent(msg_p) (((uintptr_t)(msg_p)) & 1U)
#define is_msg_past(msg_p) (!(((uintptr_t)(msg_p)) & 3U))
#define unmark_msg(msg_p) \
	((struct lp_msg *)(((uintptr_t)(msg_p)) & (UINTPTR_MAX - 3)))

extern void ScheduleNewEvent_parallel(lp_id_t receiver, simtime_t timestamp,
    unsigned event_type, const void *payload, unsigned payload_size);

extern void process_global_init(void);
extern void process_global_fini(void);

extern void process_init(void);
extern void process_fini(void);

extern void process_lp_init(void);
extern void process_lp_deinit(void);
extern void process_lp_fini(void);

extern void process_msg(void);
