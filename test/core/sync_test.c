#include <test.h>

#include <core/core.h>
#include <core/sync.h>

#include <stdatomic.h>

static atomic_uint counter;

#define THREAD_CNT 2

static int sync_test(void)
{
	int ret = 0;
	unsigned i = THREAD_CNT * 100000;
	while(i--){
		atomic_fetch_add_explicit(&counter, 1U, memory_order_relaxed);
		sync_thread_barrier();
		unsigned val = atomic_load_explicit(&counter, memory_order_relaxed);
		sync_thread_barrier();
		ret -= val % THREAD_CNT;
	}
	return ret;
}

const struct test_config test_config = {
	.threads_count = THREAD_CNT,
	.test_fnc = sync_test,
};
