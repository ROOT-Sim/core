/**
 * @file init.c
 *
 * @brief Initialization routines
 *
 * This module implements the simulator initialization routines
 *
 * SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
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
 * @brief Print a fancy ROOT-Sim logo
 */
static void print_logo(void)
{
	fprintf(stderr, "\x1b[94m   __ \x1b[90m __   _______   \x1b[94m  _ \x1b[90m       \n");
	fprintf(stderr, "\x1b[94m  /__)\x1b[90m/  ) /  ) /  __ \x1b[94m ( `\x1b[90m . ___ \n");
	fprintf(stderr, "\x1b[94m / \\ \x1b[90m(__/ (__/ (      \x1b[94m._)\x1b[90m / / / )\n");
	fprintf(stderr, "\x1b[0m\n");
}

/**
 * @brief Pretty print ROOT-Sim current configuration
 */
static void print_config(void)
{
	fprintf(stderr, "\x1b[32m");
	fprintf(stderr, "ROOT-Sim configuration:\n");
	fprintf(stderr, "\x1b[90m");

	fprintf(stderr, "Logical processes: %" PRIu64 "\n", global_config.lps);

	if(global_config.termination_time == SIMTIME_MAX)
		fprintf(stderr, "Termination time: not set\n");
	else
		fprintf(stderr, "Termination time: %lf\n", global_config.termination_time);

	if(global_config.serial) {
		fprintf(stderr, "Parallelism: sequential simulation\n");
	} else {
		if(n_nodes > 1)
			fprintf(stderr, "Parallelism: %d MPI processes\n", n_nodes);
		else
			fprintf(stderr, "Parallelism: %u threads\n", global_config.n_threads);

		if(global_config.ckpt_interval)
			fprintf(stderr, "Checkpoint interval: %u events\n", global_config.ckpt_interval);
		else
			fprintf(stderr, "Checkpoint interval: auto\n");
	}
	fprintf(stderr, "Thread-to-core binding: %sabled\n", global_config.core_binding ? "en" : "dis");

	fprintf(stderr, "GVT period: %u ms\n", global_config.gvt_period / 1000);

	fprintf(stderr, "\x1b[39m\n");
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
	memcpy(&global_config, conf, sizeof(global_config));

	if(unlikely(global_config.lps == 0)) {
		fprintf(stderr, "You must specify the total number of Logical Processes\n");
		return -1;
	}

	if(unlikely(global_config.dispatcher == NULL)) {
		fprintf(stderr, "Function pointer not correctly set\n");
		return -1;
	}

	if(unlikely(global_config.n_threads > thread_cores_count())) {
		fprintf(stderr, "Demanding %u cores, which are more than available (%u)\n",
		    global_config.n_threads, thread_cores_count());
		return -1;
	}

	log_init(global_config.logfile);

	if (global_config.serial)
		global_config.n_threads = 1;
	else if (global_config.n_threads == 0)
		global_config.n_threads = thread_cores_count();

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
	if(!configuration_done)
		return -1;

	if(!global_config.serial)
		mpi_global_init(NULL, NULL);

	if(global_config.log_level < LOG_SILENT && !tid) {
		print_logo();
		print_config();
	}

	if(global_config.serial)
		return serial_simulation();

	int ret = parallel_simulation();
	mpi_global_fini();
	return ret;
}
