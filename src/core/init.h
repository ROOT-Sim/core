/**
* @file core/init.h
*
* @brief Initialization routines
*
* This module implements the simulator initialization routines
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
#include <log/log.h>

#include <stdbool.h>
#include <stdint.h>

/// A set of configurable values used by other modules
struct simulation_configuration {
	/// The target termination logical time
	simtime_t termination_time;
	/// The gvt period expressed in microseconds
	unsigned gvt_period;
	/// The log verbosity level
	int verbosity;
	/// If set, worker threads are bound to physical cores
	bool core_binding;
	/// If set, the simulation will run on the serial runtime
	bool is_serial;
};

/// The configuration filled in by init_args_parse()
extern struct simulation_configuration global_config;

extern void init_args_parse(int argc, char **argv);
