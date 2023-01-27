/**
 * @file init.c
 *
 * @brief Initialization routines
 *
 * This module implements the simulator initialization routines
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <arch/thread.h>
#include <core/core.h>
#include <distributed/mpi.h>
#include <log/log.h>
#include <parallel/parallel.h>
#include <serial/serial.h>

#include <ROOT-Sim.h>

#include <inttypes.h>
#include <string.h>

/// A flag to check if the core library has been initialized correctly
static bool configuration_done = false;

/// The global configuration of the simulation, passed by the model
struct simulation_configuration global_config = {0};

/**
 * @brief Prints a fancy ROOT-Sim logo
 */
static void print_logo(void)
{
	fprintf(stderr, "\x1b[94m   __ \x1b[90m __   _______   \x1b[94m  _ \x1b[90m       \n");
	fprintf(stderr, "\x1b[94m  /__)\x1b[90m/  ) /  ) /  __ \x1b[94m ( `\x1b[90m . ___ \n");
	fprintf(stderr, "\x1b[94m / \\ \x1b[90m(__/ (__/ (      \x1b[94m._)\x1b[90m / / / )\n");
	fprintf(stderr, "\x1b[0m\n");
}


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
			fprintf(stderr, "Parallelism: %u threads\n", global_config.n_threads + global_config.n_threads_racer);
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

/**
 * @brief Initialize the core library
 *
 * This function must be invoked so as to initialize the core. The structure passed to this function is
 * copied into a library variable, that is used by the core to support the simulation run.
 *
 * @param conf A pointer to a struct simulation_configuration used to configure the core library.
 * @return zero if the configuration is successful, non-zero otherwise.
 */
int RootsimInit(const struct simulation_configuration *conf)
{
	memcpy(&global_config, conf, sizeof(struct simulation_configuration));

	if(unlikely(global_config.lps == 0 && global_config.lps_racer == 0)) {
		fprintf(stderr, "You must specify the total number of Logical Processes\n");
		return -1;
	}

	if(unlikely(global_config.dispatcher == NULL || global_config.committed == NULL)) {
		fprintf(stderr, "Function pointers not correctly set\n");
		return -1;
	}

	if(unlikely(global_config.n_threads + global_config.n_threads_racer > thread_cores_count())) {
		fprintf(stderr, "Demanding %u cores, which are more than available (%u)\n", global_config.n_threads,
		    thread_cores_count());
		return -1;
	}

	if(global_config.serial)
		global_config.n_threads = 1;

	if(unlikely((!global_config.lps != !global_config.n_threads) ||
		    (!global_config.lps_racer != !global_config.n_threads_racer))) {
		fprintf(stderr, "Inconsistent window racer settings\n");
		return -1;
	}

	log_init(global_config.logfile);

	if(global_config.termination_time == 0)
		global_config.termination_time = SIMTIME_MAX;

	configuration_done = true;
	return 0;
}

/**
 * @brief Start the simulation
 *
 * This function starts the simulation. It must be called *after* having initialized the ROOT-Sim core
 * by calling RootsimInit(), otherwise the invocation will fail.
 *
 * @return zero on successful simulation completion, non-zero otherwise.
 */
int RootsimRun(void)
{
	int ret;

	if(!configuration_done)
		return -1;

	if(!global_config.serial)
		mpi_global_init(NULL, NULL);

	if(global_config.log_level < LOG_SILENT && !rid) {
		print_logo();
		print_config();
	}

	if(global_config.serial) {
		ret = serial_simulation();
	} else {
		ret = parallel_simulation();
		mpi_global_fini();
	}

	return ret;
}
