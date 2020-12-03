/**
* @file serial/serial.h
*
* @brief Sequential simlation engine
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
#include <lib/lib.h>

struct s_lp_ctx {
	struct lib_ctx lib_ctx;
#if LOG_DEBUG >= LOG_LEVEL
	simtime_t last_evt_time;
#endif
	bool terminating;
};

extern struct s_lp_ctx *s_lps;
extern struct s_lp_ctx *s_current_lp;

extern void serial_simulation(void);

extern void ScheduleNewEvent(lp_id_t receiver, simtime_t timestamp,
	unsigned event_type, const void *payload, unsigned payload_size);

__attribute__ ((pure)) extern lp_id_t lp_id_get(void);
__attribute__ ((pure)) extern struct lib_ctx *lib_ctx_get(void);
