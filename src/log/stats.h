/**
 * @file log/stats.h
 *
 * @brief Statistics module
 *
 * All the facilities to collect, gather, and dump statistics are implemented
 * in this module.
 *
 * SPDX-FileCopyrightText: 2008-2020 HPDCS Group <piccione@diag.uniroma1.it>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <core/core.h>

enum stats_global_time {
	STATS_GLOBAL_INIT_END,
	STATS_GLOBAL_EVENTS_START,
	STATS_GLOBAL_EVENTS_END,
	STATS_GLOBAL_FINI_START,
	STATS_GLOBAL_COUNT
};

enum stats_time {
	STATS_MSG_PROCESSED,
	STATS_GVT,
	STATS_ROLLBACK,
	STATS_MSG_SILENT,
	STATS_COUNT
};

extern void stats_global_time_start(void);
extern void stats_global_time_take(enum stats_global_time this_stat);

extern void stats_global_init(void);
extern void stats_global_fini(void);
extern void stats_init(void);

extern void stats_time_start(enum stats_time this_stat);
extern void stats_time_take(enum stats_time this_stat);
extern void stats_on_gvt(simtime_t current_gvt);
extern void stats_dump(void);
