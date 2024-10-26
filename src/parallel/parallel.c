/**
 * @file parallel/parallel.c
 *
 * @brief Concurrent simulation engine
 *
 * SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <parallel/parallel.h>

#include <arch/thread.h>
#include <core/core.h>
#include <core/sync.h>
#include <datatypes/msg_queue.h>
#include <distributed/distributed_mem.h>
#include <distributed/mpi.h>
#include <gvt/fossil.h>
#include <gvt/gvt.h>
#include <gvt/termination.h>
#include <log/stats.h>
#include <mm/msg_allocator.h>
#include <rebind/rebind.h>

/**
 * @brief Set the affinity of the current worker thread to the core it is supposed to run on
 *
 * This is a no-op if core binding is not enabled in the configuration.
 */
static void worker_affinity_set(void)
{
	if(!global_config.core_binding)
		return;

	switch(thread_affinity_self_set(tid)) {
		case THREAD_AFFINITY_ERROR_RUNTIME:
			logger(LOG_WARN,
			    "Unable to set affinity on thread %u; you might have a too restrictive process mask", tid);
			break;

		case THREAD_AFFINITY_ERROR_NOT_SUPPORTED:
			logger(LOG_WARN,
			    "Unable to set affinity on thread %u; operation not supported on this platform", tid);
			break;

		default:
			break;
	}
}

/**
 * @brief Initialize the worker thread data structures
 * @param this_tid The numerical identifier of the worker thread
 */
static void worker_thread_init(tid_t this_tid)
{
	tid = this_tid;

	worker_affinity_set();
	stats_init();
	auto_ckpt_init();
	rebind_init();
	msg_allocator_init();
	msg_queue_init();
	sync_thread_barrier();
	lp_init();

	if(sync_thread_barrier())
		mpi_node_barrier();

	if(sync_thread_barrier()) {
		logger(LOG_INFO, "Starting simulation");
		stats_global_time_take(STATS_GLOBAL_EVENTS_START);
	}
}

/**
 * @brief Finalize the worker thread data structures
 */
static void worker_thread_fini(void)
{
	gvt_msg_drain();

	if(sync_thread_barrier()) {
		stats_dump();
		stats_global_time_take(STATS_GLOBAL_EVENTS_END);
		logger(LOG_INFO, "Finalizing simulation");

		mpi_node_barrier();
	}

	lp_fini();
	msg_queue_fini();
	sync_thread_barrier();
	msg_allocator_fini();
}

/**
 * @brief The entry point of a worker thread in the parallel runtime
 * @param rid_arg The numerical identifier of the worker thread casted to a void * to adhere to threading APIs
 * @return THREAD_RET_SUCCESS on success, THREAD_RET_ERROR on error
 *
 * This function is the entry point of every worker thread. It is responsible for initializing the worker thread data
 * structures, processing messages, and, finally, finalizing the worker thread data structures.
 */
static thrd_ret_t THREAD_CALL_CONV parallel_thread_run(void *rid_arg)
{
	worker_thread_init((uintptr_t)rid_arg);

	while(true) {
		mpi_remote_msg_handle();

		for(unsigned i = 64; i--;) {
			struct lp_msg *msg = msg_queue_extract();
			if(likely(msg))
				process_msg(msg);
		}

		simtime_t current_gvt = gvt_phase_run();
		if(unlikely(current_gvt != 0.0)) {
			auto_ckpt_on_gvt();
			msg_allocator_on_gvt();
			fossil_on_gvt(current_gvt);
			stats_on_gvt(current_gvt);
			rebind_on_gvt();
			if(unlikely(termination_on_gvt(current_gvt)))
				break;
		}
	}

	worker_thread_fini();

	return THREAD_RET_SUCCESS;
}

/**
 * @brief Initialize the globally available data structures of the parallel runtime
 */
static void parallel_global_init(void)
{
	stats_global_init();
	distributed_mem_global_init();
	lp_global_init();
	msg_queue_global_init();
	gvt_global_init();
	rebind_global_init();
}

/**
 * @brief Finalize the globally available data structures of the parallel runtime
 */
static void parallel_global_fini(void)
{
	rebind_global_fini();
	msg_queue_global_fini();
	lp_global_fini();
	distributed_mem_global_fini();
	stats_global_fini();
}

/**
 * @brief Start a parallel simulation
 * @return 0 on success, -1 on error
 *
 * This function starts a parallel simulation. It initializes the global data structures, creates the worker threads,
 * waits for the worker threads to finish the computation, and finally finalizes the global data structures.
 */
int parallel_simulation(void)
{
	logger(LOG_INFO, "Initializing parallel simulation");
	parallel_global_init();
	stats_global_time_take(STATS_GLOBAL_INIT_END);

	thr_id_t thrs[global_config.n_threads];
	for(tid_t i = 0; i < global_config.n_threads; ++i)
		if(thread_start(&thrs[i], parallel_thread_run, (void *)(uintptr_t)i)) {
			logger(LOG_FATAL, "Unable to create thread number %u!", i);
			abort();
		}

	for(tid_t i = 0; i < global_config.n_threads; ++i)
		thread_wait(thrs[i], NULL);

	stats_global_time_take(STATS_GLOBAL_FINI_START);
	parallel_global_fini();

	return 0;
}
