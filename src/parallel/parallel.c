/**
 * @file parallel/parallel.c
 *
 * @brief Concurrent simulation engine
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <parallel/parallel.h>

#include <arch/thread.h>
#include <core/core.h>
#include <core/init.h>
#include <core/sync.h>
#include <datatypes/msg_queue.h>
#include <distributed/mpi.h>
#include <gvt/fossil.h>
#include <gvt/gvt.h>
#include <gvt/termination.h>
#include <lib/lib.h>
#include <log/stats.h>
#include <lp/lp.h>
#include <mm/msg_allocator.h>

static thr_ret_t THREAD_CALL_CONV parallel_thread_run(void *rid_arg)
{
	rid = (uintptr_t)rid_arg;
	stats_init();
	msg_allocator_init();
	msg_queue_init();
	sync_thread_barrier();
	lp_init();
	process_init();

#ifdef ROOTSIM_MPI
	if (sync_thread_barrier())
		mpi_node_barrier();
#endif
	if (sync_thread_barrier()) {
		log_log(LOG_INFO, "Starting simulation");
		stats_global_time_take(STATS_GLOBAL_EVENTS_START);
	}

	while (likely(termination_cant_end())) {
		mpi_remote_msg_handle();

		unsigned i = 8;
		while (i--) {
			process_msg();
		}

		simtime_t current_gvt;
		if (unlikely(current_gvt = gvt_phase_run())) {
			termination_on_gvt(current_gvt);
			fossil_collect(current_gvt);
			stats_on_gvt(current_gvt);
		}
	}

	if (sync_thread_barrier()) {
		stats_dump();
		stats_global_time_take(STATS_GLOBAL_EVENTS_END);
		log_log(LOG_INFO, "Finalizing simulation");
	}

	process_fini();
	lp_fini();
	msg_queue_fini();
	sync_thread_barrier();
	msg_allocator_fini();

	return THREAD_RET_SUCCESS;
}

static void parallel_global_init(void)
{
	stats_global_init();
	lib_global_init();
	process_global_init();
	lp_global_init();
	msg_queue_global_init();
	termination_global_init();
	gvt_global_init();
}

static void parallel_global_fini(void)
{
	msg_queue_global_fini();
	lp_global_fini();
	process_global_fini();
	lib_global_fini();
	stats_global_fini();
}

void parallel_simulation(void)
{
	log_log(LOG_INFO, "Initializing parallel simulation");
	parallel_global_init();
	stats_global_time_take(STATS_GLOBAL_INIT_END);

	thr_id_t thrs[n_threads];
	rid_t i = n_threads;
	while (i--) {
		if (thread_start(&thrs[i], parallel_thread_run, (void *)(uintptr_t)i)) {
			log_log(LOG_FATAL, "Unable to create a thread!");
			abort();
		}
		if (global_config.core_binding && thread_affinity_set(thrs[i], i)) {
			log_log(LOG_FATAL, "Unable to set a thread affinity!");
			abort();
		}
	}

	i = n_threads;
	while (i--)
		thread_wait(thrs[i], NULL);

	stats_global_time_take(STATS_GLOBAL_FINI_START);
	parallel_global_fini();
}
