/**
 * @file parallel/parallel.c
 *
 * @brief Concurrent simulation engine
 *
 * SPDX-FileCopyrightText: 2008-2023 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <parallel/parallel.h>

#include <arch/thread.h>
#include <core/core.h>
#include <core/sync.h>
#include <cuda/gpu.h>
#include <datatypes/msg_queue.h>
#include <distributed/mpi.h>
#include <gvt/fossil.h>
#include <log/stats.h>
#include <mm/msg_allocator.h>
#include <ftl/ftl.h>

static void worker_thread_init(rid_t this_rid)
{
	rid = this_rid;
	stats_init();
	auto_ckpt_init();
	msg_allocator_init();
	msg_queue_init();
	sync_thread_barrier();
	lp_init();

	if(sync_thread_barrier()) {
		mpi_node_barrier();
		lp_initialized_set();
	}

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

	lp_fini();
	msg_queue_fini();
	sync_thread_barrier();
	msg_allocator_fini();
}

static thrd_ret_t THREAD_CALL_CONV parallel_thread_run(void *rid_arg)
{
	worker_thread_init((uintptr_t)rid_arg);
	follow_the_leader(0);	

	while(!sim_can_end()) {
		if(!termination_cant_end()) cpu_ended();
		mpi_remote_msg_handle();

		unsigned i = 64;
		while(i--)
			process_msg();

		simtime_t current_gvt = gvt_phase_run();
		if(unlikely(current_gvt != 0.0)) {
			stats_on_gvt(current_gvt);
			follow_the_leader(current_gvt);	
			termination_on_gvt(current_gvt);
			auto_ckpt_on_gvt();
			fossil_on_gvt(current_gvt);
			msg_allocator_on_gvt(current_gvt);
		}
	}

	worker_thread_fini();

	return THREAD_RET_SUCCESS;
}

static void parallel_global_init(void)
{
	stats_global_init();
	lp_global_init();
	msg_queue_global_init();
	termination_global_init();
	gvt_global_init();
}

static void parallel_global_fini(void)
{
	msg_queue_global_fini();
	lp_global_fini();
	stats_global_fini();
}

int parallel_simulation(void)
{
	logger(LOG_INFO, "Initializing parallel simulation");
	parallel_global_init();
	stats_global_time_take(STATS_GLOBAL_INIT_END);

#ifdef HAVE_CUDA
	
	thr_id_t gpu_thread;
	if(global_config.use_gpu){
		gpu_configure(global_config.lps);
		set_gpu_rid(global_config.n_threads);
		thread_start(&gpu_thread, gpu_main_loop, (void *)(uintptr_t)global_config.n_threads);
	}
#endif
	

	thr_id_t thrs[global_config.n_threads];
	rid_t i = global_config.n_threads;
	if(global_config.use_cpu){
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
	}
	
#ifdef HAVE_CUDA
	if(global_config.use_gpu){
		thread_wait(gpu_thread, NULL);
		gpu_stop();
	}
#endif

	if(global_config.use_cpu){
		stats_global_time_take(STATS_GLOBAL_FINI_START);
		parallel_global_fini();
	}
	return 0;
}
