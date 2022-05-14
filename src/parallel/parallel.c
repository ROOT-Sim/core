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
#include <core/sync.h>
#include <datatypes/msg_queue.h>
#include <distributed/mpi.h>
#include <gvt/gvt.h>
#include <gvt/termination.h>
#include <lib/lib.h>
#include <log/stats.h>
#include <lp/lp.h>
#include <mm/auto_ckpt.h>
#include <mm/model_allocator.h>
#include <mm/msg_allocator.h>

static void worker_thread_init(rid_t this_rid)
{
	rid = this_rid;
	stats_init();
	auto_ckpt_init();
	msg_allocator_init();
	msg_queue_init();
	sync_thread_barrier();
	lp_init();
	process_init();

	if(sync_thread_barrier())
		mpi_node_barrier();

	if(sync_thread_barrier()) {
		logger(LOG_INFO, "Starting simulation");
		stats_global_time_take(STATS_GLOBAL_EVENTS_START);
	}
}

static void worker_thread_fini(void)
{
	gvt_msg_drain();

	if(sync_thread_barrier()) {
		stats_dump();
		stats_global_time_take(STATS_GLOBAL_EVENTS_END);
		logger(LOG_INFO, "Finalizing simulation");

		mpi_node_barrier();
	}

	process_fini();
	lp_fini();
	msg_queue_fini();
	sync_thread_barrier();
	msg_allocator_fini();
}

static thrd_ret_t THREAD_CALL_CONV parallel_thread_run(void *rid_arg)
{
	worker_thread_init((uintptr_t)rid_arg);

	while(likely(termination_cant_end())) {
		mpi_remote_msg_handle();

		unsigned i = 64;
		while(i--) {
			process_msg();
		}

		simtime_t current_gvt;
		if(unlikely((current_gvt = gvt_phase_run()) > 0)) {
			termination_on_gvt(current_gvt);
			auto_ckpt_on_gvt();
			lp_on_gvt(current_gvt);
			msg_allocator_on_gvt(current_gvt);
			stats_on_gvt(current_gvt);
		}
	}

	worker_thread_fini();

	return THREAD_RET_SUCCESS;
}

static void parallel_global_init(void)
{
	stats_global_init();
	lib_global_init();
	lp_global_init();
	msg_queue_global_init();
	termination_global_init();
	gvt_global_init();
}

static void parallel_global_fini(void)
{
	msg_queue_global_fini();
	lp_global_fini();
	lib_global_fini();
	stats_global_fini();
}

int parallel_simulation(void)
{
	logger(LOG_INFO, "Initializing parallel simulation");
	parallel_global_init();
	stats_global_time_take(STATS_GLOBAL_INIT_END);

	thr_id_t thrs[global_config.n_threads];
	rid_t i = global_config.n_threads;
	while(i--) {
		if(thread_start(&thrs[i], parallel_thread_run, (void *)(uintptr_t)i)) {
			logger(LOG_FATAL, "Unable to create threads!");
			abort();
		}
		if(global_config.core_binding && thread_affinity_set(thrs[i], i)) {
			logger(LOG_FATAL, "Unable to set thread affinity!");
			abort();
		}
	}

	i = global_config.n_threads;
	while(i--)
		thread_wait(thrs[i], NULL);

	stats_global_time_take(STATS_GLOBAL_FINI_START);
	parallel_global_fini();

	return 0;
}
