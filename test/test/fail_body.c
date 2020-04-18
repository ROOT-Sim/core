#include <test.h>

#include <stdatomic.h>

#define THREADS_CNT 4

static atomic_uint cnt = THREADS_CNT;

static int fail_test(void)
{
	if(atomic_fetch_sub_explicit(&cnt, 1U, memory_order_relaxed) == 1){
		return -1;
	}
	return 0;
}

const struct _test_config_t test_config = {
	.test_name = "fail body",
	.threads_count = THREADS_CNT,
	.test_fnc = fail_test
};
