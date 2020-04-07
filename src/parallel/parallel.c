#include <parallel/parallel.h>

#include <arch/arch.h>
#include <core/core.h>
#include <core/init.h>
#include <core/sync.h>
#include <datatypes/msg_queue.h>
#include <lp/lp.h>
#include <gvt/gvt.h>
#include <gvt/termination.h>
#include <lib/lib.h>

#include <unistd.h>

void *parallel_thread_run(void *unused)
{
	core_init();
	msg_queue_init();
	sync_thread_barrier();
	lp_init();
	sync_thread_barrier();

	while(likely(termination_cant_end())){
		msg_queue_extract();
		process_msg();
		if(gvt_msg_processed()){
			printf("%lf barrier\n", current_gvt);
			termination_on_gvt();
		}
	}

	lp_fini();
	msg_queue_fini();
	return NULL;
}

void parallel_global_init(void)
{
	lp_global_init();
	msg_queue_global_init();
	termination_global_init();
	gvt_global_init();
	sync_global_init();

	// TODO
}

void parallel_global_fini(void)
{
	gvt_global_fini();
	msg_queue_global_fini();
	lp_global_fini();
	// TODO
}

int main(int argc, char **argv)
{
#ifdef HAVE_MPI
	mpi_global_init(&argc, &argv);
#endif

	init_args_parse(argc, argv);

	parallel_global_init();

	arch_thread_create(n_threads, true, parallel_thread_run, NULL);

	arch_thread_wait(n_threads);

	parallel_global_fini();
}
