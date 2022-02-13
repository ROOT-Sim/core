#include <test.h>

#define N_THREADS 10
#define REP_COUNT 10000

_Atomic int count = 0;

test_ret_t thread(__unused void *_)
{
	count++;
	return 0;
}

#define DESC_LEN 128
int main(void)
{
	char desc[DESC_LEN];

	init(N_THREADS);
	for(int i = 1; i < REP_COUNT+1; i++) {
		snprintf(desc, DESC_LEN, "Testing thread execution phase %d", i);
		parallel_test(desc, thread, NULL);
		assert(count == N_THREADS * i);
	}
	finish();
}
