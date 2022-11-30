/**
 * @file log/stats.h
 *
 * @brief Statistics module
 *
 * All the facilities to collect, gather, and dump statistics are implemented in this module.
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <core/core.h>

/// The kind of timestamps collected during the simulation execution lifetime
enum stats_global_type {
	/// Collected when the global data structures of the simulation framework have been initialized
	STATS_GLOBAL_INIT_END,
	/// Collected just before actual event processing activities start
	STATS_GLOBAL_EVENTS_START,
	/// Collected right after the last processed message
	STATS_GLOBAL_EVENTS_END,
	/// Collected when beginning to de-initialize global data structures
	STATS_GLOBAL_FINI_START,
	/// Collected at the last possible moment before having to dump statistics
	STATS_GLOBAL_END, // used internally, don't use elsewhere
	/// Collected at the last possible moment before having to dump statistics (high resolution timer)
	STATS_GLOBAL_HR_TOTAL, // used internally, don't use elsewhere
	/// Used to count the members of this enum
	STATS_GLOBAL_COUNT
};

/// The kind of samples collected during a simulation run
/** Time samples are collected using high resolution timers */
enum stats_thread_type {
	/// The count of processed messages
	STATS_MSG_PROCESSED,
	/// The time spent inside the model dispatcher function
	STATS_MSG_PROCESSED_TIME,
	/// The count of rollbacks
	STATS_ROLLBACK,
	/// The time spent for recovery from a rollback: checkpoint restore and anti-message sending activities
	STATS_RECOVERY_TIME,
	/// The count of rollbacked message, i.e. the already processed messages whose effect has been invalidated
	STATS_MSG_ROLLBACK,
	/// The count of taken checkpoints
	STATS_CKPT,
	/// The time spent in checkpointing activities
	STATS_CKPT_TIME,
	/// The size of LPs checkpoints
	STATS_CKPT_SIZE,
	/// The count of messages processed in coasting forward, i.e. silently executed messages
	STATS_MSG_SILENT,
	/// The time taken to carry out silent processing activities
	STATS_MSG_SILENT_TIME,
	/// The count of generated anti-messages
	STATS_MSG_ANTI,
	/// The real time elapsed since last GVT computation
	STATS_REAL_TIME_GVT, // used internally, don't use elsewhere
	/// Used to count the members of this enum
	STATS_COUNT
};

extern void stats_global_time_take(enum stats_global_type this_stat);

extern void stats_global_init(void);
extern void stats_global_fini(void);
extern void stats_init(void);

extern void stats_take(enum stats_thread_type this_stat, uint_fast64_t c);
extern uint64_t stats_retrieve(enum stats_thread_type this_stat);
extern void stats_on_gvt(simtime_t current_gvt);
extern void stats_dump(void);
