#include <parallel/parallel.h>

#include <lib/lib.h>

#include <pthread.h>

__thread unsigned tid;

void parallel_global_init(void)
{
	msg_queue_global_init();
	lib_global_init();
	// TODO
}

void parallel_global_fini(void)
{
	msg_queue_global_fini();
	lib_global_fini();
	// TODO
}

int main(int argc, char **argv)
{
	parallel_global_init();

	parallel_thread_create();
}
