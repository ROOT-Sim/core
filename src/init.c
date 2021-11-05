/**
 * @file init.c
 *
 * @brief Initialization routines
 *
 * This module implements the simulator initialization routines
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <inttypes.h>

#include <arch/io.h>
#include <core/core.h>
#include <ROOT-Sim.h>
#include <string.h>
#include <log/stats.h>
#include <distributed/mpi.h>
#include <serial/serial.h>
#include <parallel/parallel.h>

/// The global configuration of the simulation, passed by the model
struct simulation_configuration global_config;

/**
 * @brief Pretty prints ROOT-Sim current configuration
 */
static void print_config(void)
{
	fprintf(stderr, "\x1b[32m");
	fprintf(stderr, "ROOT-Sim configuration:\n");
	fprintf(stderr, "\x1b[90m");

	fprintf(stderr, "Logical processes: %" PRIu64 "\n", global_config.lps);
	fprintf(stderr, "Termination time: ");
	if(global_config.termination_time == SIMTIME_MAX)
		fprintf(stderr, "not set\n");
	else
		fprintf(stderr, "%lf\n", global_config.termination_time);

	if(global_config.serial) {
		fprintf(stderr, "Parallelism: sequential simulation\n");
	} else {
		if(n_nodes > 1)
			fprintf(stderr, "Parallelism: %d MPI processes\n", n_nodes);
		else
			fprintf(stderr, "Parallelism: %u threads\n", global_config.n_threads);
	}
	fprintf(stderr, "Thread-to-core binding: %s\n", global_config.core_binding ? "enabled" : "disabled");

	fprintf(stderr, "GVT period: %u ms\n", global_config.gvt_period / 1000);

	if(global_config.ckpt_interval) {
		fprintf(stderr, "Checkpoint interval: %u events\n", global_config.ckpt_interval);
	} else {
		if(!global_config.serial)
			fprintf(stderr, "Checkpoint interval: auto\n");
	}

	fprintf(stderr, "\x1b[39m");

	fprintf(stderr, "\n");
	fflush(stderr);
}

int RootsimInit(struct simulation_configuration *conf)
{
	memcpy(&global_config, conf, sizeof(struct simulation_configuration));

	// Sanity check on the number of LPs
	if(unlikely(global_config.lps == 0)) {
		fprintf(stderr, "You must specify the total number of Logical Processes\n");
		return -1;
	}

	// Sanity check on function pointers
	if(unlikely(global_config.dispatcher == NULL || global_config.committed == NULL)) {
		fprintf(stderr, "Function pointers not correctly set\n");
		return -1;
	}

	// Configure the logger
	log_init(global_config.logfile);

	// Set termination time to infinity if required
	if(global_config.termination_time == 0)
		global_config.termination_time = SIMTIME_MAX;

	return 0;
}

int RootsimRun(void)
{
	int ret;

	if(global_config.log_level > LOG_SILENT)
		print_config();

	stats_global_time_start();

	if(global_config.serial) {
		ret = serial_simulation();
	} else {
		mpi_global_init(NULL, NULL);
		ret = parallel_simulation();
		mpi_global_fini();
	}

	return ret;
}
