#include <core/core.h>

#include <stdatomic.h>
#include <setjmp.h>

lp_id_t n_lps;

#ifndef NEUROME_SERIAL

nid_t n_nodes = 1;
rid_t n_threads;
nid_t nid;
__thread rid_t rid;

static __thread jmp_buf exit_jmp_buf;

void core_init(void)
{
	static atomic_uint rid_helper = 0;
	rid = atomic_fetch_add_explicit(&rid_helper, 1U, memory_order_relaxed);

	int code = setjmp(exit_jmp_buf);
	if(code){
		//todo abort mission!
	}
}

void core_abort(void)
{
	longjmp(exit_jmp_buf, -1);
}

#endif
