/**
 * @file main.c
 *
 * @brief Simulator main entry point
 *
 * This module implements the main function, the entry point of all simulations.
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <core/core.h>

#include <core/init.h>
#include <distributed/mpi.h>
#include <log/stats.h>
#include <parallel/parallel.h>
#include <serial/serial.h>

visible int main(int argc, char **argv)
{
	stats_global_time_start();
	mpi_global_init(&argc, &argv);
	init_args_parse(argc, argv);

	if (global_config.is_serial) {
		serial_simulation();
	} else {
		parallel_simulation();
	}

	mpi_global_fini();
}
