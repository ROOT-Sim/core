/**
 * @file core/init.h
 *
 * @brief Initialization routines
 *
 * This module implements the simulator initialization routines
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
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
