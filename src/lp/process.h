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

/// An entry in the Past Event Set (see struct process_ctx)
struct pes_entry {
	/// A tagged pointer used to distinguish between different message kinds (see enum pes_entry)
	uintptr_t raw;
};

/**
 * The type of a Past Event Set entry
 * Note that the exact same message buffer can appear as part of the PES of two different LPs. */
enum pes_entry_type {
	/// The message contained in the PES entry has been sent received by the LP
	PES_ENTRY_RECEIVED = 0U,
	/// The message contained in the PES entry has been sent by the LP in shared memory
	PES_ENTRY_SENT_LOCAL = 1U,
	/// The message contained in the PES entry has been sent by the LP to another MPI rank
	PES_ENTRY_SENT_REMOTE = 2U,
};

/// The message processing data produced by the LP
struct process_ctx {
	/// The Past Event Set of this LP; the messages processed in the past by the owner LP
	array_declare(struct pes_entry) pes;
	/** The list of remote anti-messages delivered before their original counterpart.
	 *  Hopefully this is 99.9% of the time empty */
	struct lp_msg *early_antis;
	/** The current logical time at which this LP is. This is lazily updated and not always accurate;
	 * it's sufficient for faster straggler detection */
	simtime_t bound;
};

/**
 * @brief Check if pes entry corresponds to a received message
 * @param entry The pes entry
 * @return True if the entry refers to a received message, false otherwise
 */
#define pes_entry_is_received(entry) !((entry).raw & (PES_ENTRY_SENT_LOCAL | PES_ENTRY_SENT_REMOTE))

/**
 * @brief Check if pes entry corresponds to a locally sent message (i.e.: an event sent in shared memory)
 * @param entry The pes entry
 * @return True if the entry refers to a locally sent message, false otherwise.
 */
#define pes_entry_is_sent_local(entry) ((entry).raw & PES_ENTRY_SENT_LOCAL)

/**
 * @brief Check if pes entry corresponds to a remotely sent message (i.e.: an event sent to another MPI rank)
 * @param entry The pes entry
 * @return True if the entry refers to a remotely sent message, false otherwise.
 */
#define pes_entry_is_sent_remote(entry) ((entry).raw & PES_ENTRY_SENT_REMOTE)

/**
 * @brief Make a struct pes_entry from a message and an entry type
 * @param msg The message to encode in the PES entry, a pointer to a struct lp_msg
 * @param tag The message type to encode in the PES entry, a value of type enum pes_entry_type
 * @return The newly initialized struct pes_entry value
 */
#define pes_entry_make(msg, tag) ((struct pes_entry){.raw = (tag) | (uintptr_t)(msg)})

/**
 * @brief Get the message pointer out of a struct pes_entry of any kind
 * @param entry The struct pes_entry
 * @return A pointer to the message encoded in the entry, a pointer to a struct lp_msg
 */
#define pes_entry_msg(entry) (struct lp_msg *)((entry).raw & ~(uintptr_t)(PES_ENTRY_SENT_LOCAL | PES_ENTRY_SENT_REMOTE))

/**
 * @brief Get the message pointer out of a struct pes_entry of type PES_ENTRY_RECEIVED
 * @param entry The struct pes_entry
 * @return A pointer to the message encoded in the entry, a pointer to a struct lp_msg
 *
 * It is marginally more efficient than pes_entry_msg() but it will invoke undefined behaviour for non
 * PES_ENTRY_RECEIVED entry types.
 */
#define pes_entry_msg_received(entry)                                                                                  \
	__extension__({                                                                                                \
		assert(pes_entry_is_received(entry));                                                                  \
		(struct lp_msg *)(entry).raw;                                                                          \
	})

struct lp_ctx; // forward declaration

extern void process_lp_init(struct lp_ctx *lp);
extern void process_lp_fini(struct lp_ctx *lp);

extern void process_msg(struct lp_msg *msg);

extern void ScheduleNewEvent_parallel(lp_id_t receiver, simtime_t timestamp, unsigned event_type, const void *payload,
    unsigned payload_size);
