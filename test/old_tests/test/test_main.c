/**
 * @file test/test_main.c
 *
 * @brief Main stub for test
 *
 * The main function stub for tests which do not declare a main() entry point
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <test.h>

#include <arch/thread.h>

#include <limits.h>
#include <stdatomic.h>

/// The arguments passed to test threads
struct stub_arguments {
	/// The actual entry point defined by the test configuration
	int (*test_fnc)(void);
	/// The thread identifier which is set upon thread startup
	rid_t tid;
};

/**
 * @brief The entry point of the test threads
 * @arg the arguments passed by the main stub, a struct stub_arguments
 * @return THREAD_RET_SUCCESS in case of success, THREAD_RET_FAILURE otherwise
 */
static thr_ret_t THREAD_CALL_CONV test_run_stub(void *arg)
{
	struct stub_arguments *args = arg;
	rid = args->tid;
	int ret = args->test_fnc();
	return ret ? THREAD_RET_FAILURE : THREAD_RET_SUCCESS;
}

int main(int argc, char **argv)
{
	(void)argc; (void)argv;
	int ret = 0;

	rid_t t = test_config.threads_count ? test_config.threads_count :
			thread_cores_count();
	if (test_config.test_fnc == NULL)
		t = 0;

	n_threads = t;

	if (test_config.test_init_fnc && (ret = test_config.test_init_fnc())) {
		printf("Test initialization failed with code %d\n", ret);
		return ret;
	}

	thr_id_t threads[t];
	struct stub_arguments args[t];

	for (rid_t i = 0; i < t; ++i) {
		args[i].test_fnc = test_config.test_fnc;
		args[i].tid = i;
		if (thread_start(&threads[i], test_run_stub, &args[i]))
			return TEST_BAD_FAIL_EXIT_CODE;
	}

	for (rid_t i = 0; i < t; ++i) {
		thr_ret_t thr_ret;
		if (thread_wait(threads[i], &thr_ret))
			return TEST_BAD_FAIL_EXIT_CODE;

		if (thr_ret) {
			printf("Thread %u failed the test\n", i);
			return -1;
		}
	}

	if (test_config.test_fini_fnc && (ret = test_config.test_fini_fnc())) {
		printf("Test finalization failed with code %d\n", ret);
		return ret;
	}

	return 0;
}

/**
 * @brief Synchronizes threads on a barrier
 * @return true if this thread has been elected as leader, false otherwise
 *
 * This is a more battle tested although worse performing version of the thread
 * barrier. We can't rely on the pthread barrier because it's not portable.
 */
bool test_thread_barrier(void)
{
	static atomic_uint b_in, b_out, b_cr;

	unsigned i;
	unsigned count = n_threads;
	unsigned max_before_reset = (UINT_MAX / 2) - (UINT_MAX / 2) % count;
	do {
		i = atomic_fetch_add_explicit(
				&b_in, 1U, memory_order_acq_rel) + 1;
	} while (__builtin_expect(i > max_before_reset, 0));

	unsigned cr = atomic_load_explicit(&b_cr, memory_order_relaxed);

	bool leader = i == cr + count;
	if (leader)
		atomic_store_explicit(&b_cr, cr + count, memory_order_release);
	else
		while (i > cr)
			cr = atomic_load_explicit(&b_cr, memory_order_relaxed);

	atomic_thread_fence(memory_order_acquire);

	unsigned o = atomic_fetch_add_explicit(&b_out, 1,
			memory_order_release) + 1;
	if (__builtin_expect(o == max_before_reset, 0)) {
		atomic_thread_fence(memory_order_acquire);
		atomic_store_explicit(&b_cr, 0, memory_order_relaxed);
		atomic_store_explicit(&b_out, 0, memory_order_relaxed);
		atomic_store_explicit(&b_in, 0, memory_order_release);
	}
	return leader;
}
