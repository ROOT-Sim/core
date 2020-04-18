#include <core/core.h>

#include <stdatomic.h>
#include <setjmp.h>

uint64_t n_lps;

#ifndef NEUROME_SERIAL

unsigned n_nodes = 1;
unsigned n_threads;
nid_t nid;
__thread rid_t rid;

static __thread jmp_buf exit_jmp_buf;

void core_global_init(void)
{
//	sigset_t signal_set;
//	pthread_t sig_thread;
//
//	/* block all signals */
//	sigfillset( &signal_set );
//	pthread_sigmask( SIG_BLOCK, &signal_set,
//		NULL );

}

void core_init(void)
{
	static atomic_uint rid_helper = 0;
	rid = atomic_fetch_add_explicit(&rid_helper, 1U, memory_order_relaxed);

	int code = setjmp(exit_jmp_buf);
	if(code){
		//todo abort mission!
	}
}

void core_error(void)
{
	longjmp(exit_jmp_buf, -1);
}

#endif
