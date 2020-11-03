/**
* @file datatypes/remote_msg_map.h
*
* @brief Message map datatype
*
* Message map datatype
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

#include <lp/msg.h>

extern void remote_msg_map_global_init(void);
extern void remote_msg_map_global_fini(void);
extern void remote_msg_map_fossil_collect(simtime_t current_gvt);
extern void remote_msg_map_match(uintptr_t msg_id, nid_t nid, lp_msg *msg);
