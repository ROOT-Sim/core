/**
 * @file test/test_main.c
 *
 * @brief Main stub for test
 *
 * The main function stub for tests which do not declare a main() entry point
 *
 * @copyright
 * Copyright (C) 2008-2020 HPDCS Group
 * https://hpdcs.github.io
 *
 * This file is part of ROOT-Sim (ROme OpTimistic Simulator).
 *
 * ROOT-Sim is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; only version 3 of the License applies.
 *
 * ROOT-Sim is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ROOT-Sim; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include <test.h>

#include <arch/thread.h>

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
		if (thread_create(&threads[i], test_run_stub, &args[i]))
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
