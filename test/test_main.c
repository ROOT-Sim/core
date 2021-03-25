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

extern __thread rid_t rid;

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
	thr_id_t threads[test_config.threads_count];
	struct stub_arguments args[test_config.threads_count];

	if (test_config.test_init_fnc && (ret = test_config.test_init_fnc())) {
		printf("Test initialization failed with code %d\n", ret);
		return ret;
	}

	for (unsigned i = 0; i < test_config.threads_count; ++i) {
		args[i].test_fnc = test_config.test_fnc;
		args[i].tid = i;
		if (thread_start(&threads[i], test_run_stub, &args[i]))
			return TEST_BAD_FAIL_EXIT_CODE;
	}

	for (unsigned i = 0; i < test_config.threads_count; ++i) {
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
