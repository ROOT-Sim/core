/**
* @file lib/topology/topology.h
*
* @brief Topology library
*
* This library is allows models to setup and query different topologies.
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

#include <stdint.h>
#include <core/core.h>

// this initializes the topology environment
extern void topology_global_init(void);

extern __attribute__ ((pure)) uint64_t RegionsCount(void);
extern __attribute__ ((pure)) uint64_t DirectionsCount(void);
extern __attribute__ ((pure)) uint64_t GetReceiver(uint64_t from, enum _direction_t direction);

extern uint64_t FindReceiver(void);
