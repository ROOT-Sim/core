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

struct pes_entry {
	uintptr_t raw;
};

enum pes_entry_type { PES_ENTRY_RECEIVED = 0U, PES_ENTRY_SENT_LOCAL = 1U, PES_ENTRY_SENT_REMOTE = 2U };

/// The message processing data produced by the LP
struct process_ctx {
	/// The messages processed in the past by the owner LP
	array_declare(struct pes_entry) pes;
	/// The list of remote anti-messages delivered before their original counterpart
	/** Hopefully this is 99.9% of the time empty */
	struct lp_msg *early_antis;
	/// The current logical time at which this LP is
	/** This is lazily updated and not always accurate; it's sufficient for faster straggler detection */
	simtime_t bound;
};

#define pes_entry_is_received(entry) !((entry).raw & (PES_ENTRY_SENT_LOCAL | PES_ENTRY_SENT_REMOTE))
#define pes_entry_is_sent_local(entry) ((entry).raw & PES_ENTRY_SENT_LOCAL)
#define pes_entry_is_sent_remote(entry) ((entry).raw & PES_ENTRY_SENT_REMOTE)
#define pes_entry_make(msg, tag) ((struct pes_entry){.raw = (tag) | (uintptr_t)(msg)})
#define pes_entry_msg(entry) (struct lp_msg *)((entry).raw & ~(uintptr_t)(PES_ENTRY_SENT_LOCAL | PES_ENTRY_SENT_REMOTE))
#define pes_entry_msg_received(entry)                                                                                  \
	__extension__({                                                                                                \
		assert(pes_entry_is_received(entry));                                                                  \
		(struct lp_msg *)(entry).raw;                                                                          \
	})


struct lp_ctx; // forward declaration

extern void process_lp_init(struct lp_ctx *lp);
extern void process_lp_fini(struct lp_ctx *lp);

extern void process_msg(struct lp_msg *msg);
extern bool process_early_anti_message_check(struct process_ctx *proc_p, struct lp_msg *msg);

extern void ScheduleNewEvent_parallel(lp_id_t receiver, simtime_t timestamp, unsigned event_type, const void *payload,
    unsigned payload_size);
