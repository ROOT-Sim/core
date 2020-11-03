/**
* @file parallel/parallel.c
*
* @brief Concurrent simlation engine
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

#include <arch/arch.h>
#include <core/core.h>
#include <core/init.h>
#include <core/sync.h>
#include <datatypes/msg_queue.h>
#include <log/stats.h>
#include <lp/lp.h>
#include <mm/msg_allocator.h>
#include <gvt/fossil.h>
#include <gvt/gvt.h>
#include <gvt/termination.h>
#include <lib/lib.h>

#include <unistd.h>

void *parallel_thread_run(void *unused)
{
	(void)unused;

	core_init();
	msg_allocator_init();
	msg_queue_init();
	sync_thread_barrier();
	lp_init();
	sync_thread_barrier();

	if(!rid) log_log(LOG_INFO, "Starting simulation");

	while(likely(termination_cant_end())){
#ifdef NEUROME_MPI
		mpi_remote_msg_handle();
#endif
		unsigned i = 8;
		while(i--){
			process_msg();
		}

		simtime_t current_gvt;
		if(unlikely(current_gvt = gvt_msg_processed())){
			termination_on_gvt(current_gvt);
			fossil_collect(current_gvt);
			stats_progress_print(current_gvt);
		}
	}

	stats_dump();

	if(!rid) log_log(LOG_INFO, "Finalizing simulation");

	lp_fini();

	msg_queue_fini();
	sync_thread_barrier();
	msg_allocator_fini();
	return NULL;
}

void parallel_global_init(void)
{
	lp_global_init();
	msg_queue_global_init();
	termination_global_init();
	gvt_global_init();
	lib_global_init();
#ifdef NEUROME_MPI
	remote_msg_map_global_init();
#endif
}

void parallel_global_fini(void)
{
#ifdef NEUROME_MPI
	remote_msg_map_global_fini();
#endif
	lib_global_fini();
	msg_queue_global_fini();
	lp_global_fini();
}

int main(int argc, char **argv)
{
#ifdef NEUROME_MPI
	mpi_global_init(&argc, &argv);
#endif
	init_args_parse(argc, argv);

	log_log(LOG_INFO, "Initializing parallel simulation");

	parallel_global_init();

	arch_thread_create(n_threads, global_config.core_binding,
		parallel_thread_run, NULL);

	arch_thread_wait(n_threads);

	parallel_global_fini();

#ifdef NEUROME_MPI
	mpi_global_fini();
#endif
}
