/**
* @file gvt/termination.h
*
* @brief Termination detection module
*
* This module implements the termination detection checks.
*
* @copyright
* Copyright (C) 2008-2021 HPDCS Group
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

#include <stdatomic.h>

extern atomic_int nodes_to_end;

#define termination_cant_end() (atomic_load_explicit(&nodes_to_end, memory_order_relaxed) > 0)

extern void termination_global_init(void);
extern void termination_lp_init(void);
extern void termination_on_msg_process(simtime_t msg_time);
extern void termination_on_gvt(simtime_t current_gvt);
extern void termination_on_lp_rollback(simtime_t msg_time);
extern void termination_on_ctrl_msg(void);
extern void termination_force(void);

