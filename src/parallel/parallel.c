#include <parallel/parallel.h>

#include <core/core.h>
#include <lib/lib.h>

#include <pthread.h>

void parallel_thread_run(void *unused)
{
	sync_init();

	while(1){
		msg_queue_extract();

	}

}

void parallel_global_init(void)
{
	msg_queue_global_init();
	lib_global_init();


	// TODO
}

void parallel_global_fini(void)
{
	lib_global_fini();
	msg_queue_global_fini();
	// TODO
}

int main(int argc, char **argv)
{
#ifdef HAVE_MPI
	mpi_global_init(&argc, &argv);
#endif

	init_args_parse(argc, argv);

	parallel_global_init();

	arch_thread_init(n_threads, parallel_thread_run, NULL);

	sleep(100);
}
