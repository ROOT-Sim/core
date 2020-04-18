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
		msg_queue_extract();
		process_msg();
		if(gvt_msg_processed()){
			termination_on_gvt();
			stats_progress_print();
			fossil_collect();
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
}

void parallel_global_fini(void)
{
	lib_global_fini();
	gvt_global_fini();
	msg_queue_global_fini();
	lp_global_fini();
}

int main(int argc, char **argv)
{
#ifdef HAVE_MPI
	mpi_global_init(&argc, &argv);
#endif
	init_args_parse(argc, argv);

	log_log(LOG_INFO, "Initializing parallel simulation");

	parallel_global_init();

	arch_thread_create(n_threads, true, parallel_thread_run, NULL);

	arch_thread_wait(n_threads);

	parallel_global_fini();
}
