/**
* @file core/init.h
*
* @brief Initialization routines
*
* This module implements the simulator initialization routines
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
#include <stdbool.h>

#include <core/core.h>
#include <log/log.h>

typedef struct {
	int verbosity; //!< the log verbosity level
	simtime_t termination_time; //!< the logical target termination time
	unsigned gvt_period; //!< the gvt period expressed in microseconds
#ifndef ROOTSIM_SERIAL
	bool core_binding; //!< if set, worker threads are bound to physical cores
#endif
} simulation_configuration;

extern simulation_configuration global_config;

extern void init_args_parse(int argc, char **argv);
