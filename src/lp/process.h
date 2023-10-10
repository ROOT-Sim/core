/**
 * @file lp/process.h
 *
 * @brief LP state management functions
 *
 * LP state management functions
 *
 * SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <datatypes/array.h>
#include <lp/msg.h>

/// The message processing data produced by the LP
struct process_ctx {
	/// The messages processed in the past by the owner LP
	array_declare(struct pes_entry) p_msgs;
	/// The list of remote anti-messages delivered before their original counterpart
	/** Hopefully this is 99.9% of the time empty */
	struct lp_msg *early_antis;
	/// The current logical time at which this LP is
	/** This is lazily updated and not always accurate; it's sufficient for faster straggler detection */
	simtime_t bound;
};

#define proc_is_sent(msg_p) (((uintptr_t)(msg_p)) & 3U)
#define proc_is_sent_local(msg_p) (((uintptr_t)(msg_p)) & 1U)
#define proc_is_sent_remote(msg_p) (((uintptr_t)(msg_p)) & 2U)
#define proc_untagged(msg_p) ((struct lp_msg *)(((uintptr_t)(msg_p)) & (UINTPTR_MAX - 3U)))

struct lp_ctx; // forward declaration

extern void process_lp_init(struct lp_ctx *lp);
extern void process_lp_fini(struct lp_ctx *lp);

extern void process_msg(struct lp_msg *msg);

extern void ScheduleNewEvent_parallel(lp_id_t receiver, simtime_t timestamp, unsigned event_type, const void *payload,
    unsigned payload_size);
