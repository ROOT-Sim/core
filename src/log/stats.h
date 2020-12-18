/**
 * @file log/stats.h
 *
 * @brief Statistics module
 *
 * All facitilies to collect, gather, and dump statistics are implemented
 * in this module.
 *
 * @copyright
 * Copyright (C) 2008-2020 HPDCS Group
 * https://hpdcs.github.io
 *
 * This file is part of ROOT-Sim (ROme OpTimistic Simulator).
 *
 * ROOT-Sim is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; only version 3 of the License applies.
 *
 * ROOT-Sim is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ROOT-Sim; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#pragma once

#include <core/core.h>

enum stats_time_t {
	STATS_MSG_PROCESSED,
	STATS_GVT,
	STATS_ROLLBACK,
	STATS_MSG_SILENT,
	STATS_NUM
};

extern void stats_global_init(void);
extern void stats_global_fini(void);
extern void stats_init(void);
extern void stats_fini(void);

extern void stats_time_start(enum stats_time_t this_stat);
extern void stats_time_take(enum stats_time_t this_stat);
extern void stats_on_gvt(simtime_t current_gvt);
extern void stats_dump(void);
