/**
* @file parallel/parallel.c
*
* @brief Concurrent simulation engine
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
#include <parallel/parallel.h>

#include <arch/thread.h>
#include <core/core.h>
#include <core/init.h>
#include <core/sync.h>
#include <datatypes/msg_queue.h>
#include <datatypes/remote_msg_map.h>
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
	sync_thread_barrier();

	if (!rid)
		log_log(LOG_INFO, "Starting simulation");

	while (likely(termination_cant_end())) {
#ifdef ROOTSIM_MPI
		mpi_remote_msg_handle();
#endif
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

	if (!rid) {
		stats_dump();
		log_log(LOG_INFO, "Finalizing simulation");
	}

	lp_fini();

	msg_queue_fini();
	sync_thread_barrier();
	msg_allocator_fini();
	stats_fini();

	return THREAD_RET_SUCCESS;
}

static void parallel_global_init(void)
{
	stats_global_init();
	lp_global_init();
	msg_queue_global_init();
	termination_global_init();
	gvt_global_init();
	lib_global_init();
#ifdef ROOTSIM_MPI
	remote_msg_map_global_init();
#endif
}

static void parallel_global_fini(void)
{
#ifdef ROOTSIM_MPI
	remote_msg_map_global_fini();
#endif
	lib_global_fini();
	msg_queue_global_fini();
	lp_global_fini();
	stats_global_fini();
}

void parallel_simulation(void)
{
	log_log(LOG_INFO, "Initializing parallel simulation");

	parallel_global_init();

	thr_id_t thrs[n_threads];
	rid_t i = n_threads;
	while (i--) {
		if (thread_create(&thrs[i], parallel_thread_run,
				  (void *)(uintptr_t)i)) {
			log_log(LOG_FATAL, "Unable to create a thread!");
			abort();
		}
		if (global_config.core_binding &&
				thread_affinity_set(thrs[i], i)) {
			log_log(LOG_FATAL, "Unable to set a thread affinity!");
			abort();
		}
	}

	i = n_threads;
	while (i--)
		thread_wait(thrs[i], NULL);

	parallel_global_fini();
}
