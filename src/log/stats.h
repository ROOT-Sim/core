/**
 * @file log/stats.h
 *
 * @brief Statistics module
 *
 * All the facilities to collect, gather, and dump statistics are implemented
 * in this module.
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <core/core.h>

enum stats_global_type {
	STATS_GLOBAL_START, // used internally, don't use elsewhere
	STATS_GLOBAL_INIT_END,
	STATS_GLOBAL_EVENTS_START,
	STATS_GLOBAL_EVENTS_END,
	STATS_GLOBAL_FINI_START,
	STATS_GLOBAL_END, // used internally, don't use elsewhere
	STATS_GLOBAL_COUNT
};

enum stats_thread_type {
	STATS_MSG_PROCESSED,
	STATS_ROLLBACK,
	STATS_MSG_ROLLBACK,
	STATS_CKPT,
	STATS_CKPT_TIME,
	STATS_MSG_SILENT,
	STATS_MSG_SILENT_TIME,
	STATS_MSG_REMOTE_RECEIVED,
	STATS_REAL_TIME_GVT, // used internally, don't use elsewhere
	STATS_MSG_PUBSUB,
	STATS_MSG_PUBSUB_ANTI,
	STATS_MSG_PUBSUB_COMMITTED,
	STATS_COUNT,
};

extern void stats_global_time_start(void);
extern void stats_global_time_take(enum stats_global_type this_stat);

extern void stats_global_init(void);
extern void stats_global_fini(void);
extern void stats_init(void);

extern void stats_take(enum stats_thread_type this_stat, unsigned c);
extern uint64_t stats_retrieve(enum stats_thread_type this_stat);
extern void stats_on_gvt(simtime_t current_gvt);
extern void stats_dump(void);
