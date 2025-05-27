/**
 * @file test/test.c
 *
 * @brief Custom minimalistic testing framework
 *
 * SPDX-FileCopyrightText: 2008-2023 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <setjmp.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#include <test.h>

/**
 * @brief Context for managing test execution.
 *
 * This structure holds the thread-local context for a test, including the
 * jump buffer for handling assertion failures, the random number generator
 * state, the thread ID, and a flag indicating if an assertion has failed.
 */
struct test_ctx {
	jmp_buf jmp_buf;       /**< Jump buffer for handling assertion failures. */
	test_rng_state rng;    /**< Random number generator state for the test. */
	unsigned tid;          /**< Thread ID for parallel test execution. */
	bool assertion_failed; /**< Flag indicating if an assertion has failed. */
};

/**
 * @brief Structure representing a single test case.
 *
 * This structure contains the function to execute as the test, its arguments,
 * a random seed for the test, and a pointer to a thread ID helper for parallel
 * execution.
 */
struct test_case {
	test_fn fn;                /**< Function pointer to the test function. */
	void *arg;                 /**< Argument to pass to the test function. */
	uint64_t rng_seed;         /**< Random seed for the test case. */
	atomic_uint *tid_helper_p; /**< Pointer to a thread ID helper for parallel execution. */
};

/** @brief Atomic flag indicating if the test setup has been completed. */
static atomic_flag test_setup_done = ATOMIC_FLAG_INIT;

/** @brief Atomic counter for the number of successful tests. */
static atomic_uint test_success_count;

/** @brief Atomic counter for the total number of tests executed. */
static atomic_uint test_total_count;

/**
 * @brief Thread-local context for the current test.
 *
 * This thread-local variable holds the context for the currently executing test,
 * including the random number generator state and assertion failure status.
 */
static _Thread_local struct test_ctx test_ctx = {.assertion_failed = false,
    .rng = (((test_rng_state)12399196U) << 64) | (test_rng_state)3532456U};

/**
 * @brief Marks the current test as failed and jumps to the failure handler.
 *
 * This function is used to handle test failures by performing a long jump
 * to the failure handler in the test context.
 *
 * @note This function does not return.
 */
__attribute__((noreturn)) void test_fail(void)
{
	longjmp(test_ctx.jmp_buf, 1);
}


/**
 * @brief Finalizes the test framework at program exit.
 *
 * This function is registered to run at program exit and provides a summary
 * of the test results. It prints the total number of tests executed, the number
 * of tests that passed, and a separator for readability. If any top-level
 * assertions failed or if the total number of tests does not match the number
 * of successful tests, the program exits with a failure status.
 */
static void test_at_exit(void)
{
	unsigned tot = atomic_load_explicit(&test_total_count, memory_order_relaxed);
	unsigned succ = atomic_load_explicit(&test_success_count, memory_order_relaxed);

	int d = snprintf(NULL, 0, "PASSED.............: %u / %u\n", succ, tot);
	printf("%.*s\n", d, "============================================================================");
	printf("PASSED.............: %u / %u\n", succ, tot);
	printf("%.*s\n", d, "============================================================================");
	fflush(stdout);

	if(test_ctx.assertion_failed) {
		printf("One or more top level assertions failed!\n");
		fflush(stdout);
		_exit(EXIT_FAILURE);
	}

	if(tot != succ)
		_exit(EXIT_FAILURE);
}


/**
 * @brief Initializes the test environment for a single test case.
 *
 * This function sets up the test environment by registering the cleanup function
 * to be executed at program exit (if not already registered) and increments the
 * total test count. It also prints the description of the test case to the standard output.
 *
 * @param desc A description of the test case being initialized.
 */
static void test_init(const char *desc)
{
	if(!atomic_flag_test_and_set_explicit(&test_setup_done, memory_order_relaxed))
		atexit(test_at_exit);
	atomic_fetch_add_explicit(&test_total_count, 1U, memory_order_relaxed);
	printf("%s... ", desc);
	fflush(stdout);
}


/**
 * @brief Executes a single test case in a thread.
 *
 * This function runs a test case in a thread, managing the test context and handling
 * any assertion failures or errors that occur during execution. It ensures that the
 * thread-local test context is properly restored after the test case completes.
 *
 * @param args A pointer to a `struct test_case` containing the test function, arguments,
 *             and thread-specific data.
 * @return THREAD_RET_SUCCESS if the test passes, or THREAD_RET_FAILURE if the test fails.
 */
static thrd_ret_t test_run(void *args)
{
	struct test_case *test = args;

	struct test_ctx old_ctx = test_ctx;
	test_ctx.assertion_failed = false;

	if(test->tid_helper_p != NULL) {
		test_ctx.tid = atomic_fetch_add_explicit(test->tid_helper_p, 1U, memory_order_relaxed);
		rng_init(&test_ctx.rng, old_ctx.rng ^ ((__uint128_t)test->rng_seed * (test_ctx.tid + 1)));
	}

	bool has_failed;
	if(setjmp(test_ctx.jmp_buf)) {
		has_failed = true;
	} else {
		has_failed = test->fn(test->arg) != 0;
	}

	has_failed |= test_ctx.assertion_failed;
	test_ctx = old_ctx;

	return has_failed ? THREAD_RET_FAILURE : THREAD_RET_SUCCESS;
}


/**
 * @brief Finalizes the result of a test case.
 *
 * This function evaluates the outcome of a test case by comparing whether the test
 * result matches the expected outcome. It updates the success count for passed tests
 * and prints the appropriate message to the standard output.
 *
 * @param has_failed A boolean indicating whether the test case has failed.
 * @param expected_fail A boolean indicating whether the test case was expected to fail.
 * @return 0 if the test result matches the expectation, or -1 otherwise.
 */
static int test_fini(const bool has_failed, const bool expected_fail)
{
	if(has_failed == expected_fail) {
		if(expected_fail)
			puts("expected fail.");
		else
			puts("passed.");

		atomic_fetch_add_explicit(&test_success_count, 1U, memory_order_relaxed);
		fflush(stdout);
		return 0;
	} else {
		if(expected_fail)
			puts("UNEXPECTED PASS.");
		else
			puts("FAILED.");
		fflush(stdout);
		return -1;
	}
}


/**
 * @brief Executes a single test case.
 *
 * This function initializes the test environment, runs the specified test case,
 * and finalizes the test by checking its result against the expected outcome.
 *
 * @param desc A description of the test being executed.
 * @param test_fn The test function to execute. It must match the `test_fn` type signature.
 * @param arg An argument to pass to the test function.
 * @param should_fail A boolean indicating whether the test is expected to fail.
 * @return 0 if the test result matches the expectation, or -1 otherwise.
 */
static int test_single(const char *desc, test_fn test_fn, void *arg, bool should_fail)
{
	test_init(desc);

	struct test_case test = {.fn = test_fn, .arg = arg, .tid_helper_p = NULL};
	bool test_failed = test_run(&test);

	return test_fini(test_failed, should_fail);
}


/**
 * @brief Executes a test function and checks its result.
 *
 * This function runs the provided test function and verifies if it passes.
 * It is used for scenarios where the test is expected to succeed.
 *
 * @param desc A description of the test being executed.
 * @param test_fn The test function to execute. It must match the `test_fn` type signature.
 * @param arg An argument to pass to the test function.
 * @return 0 if the test passes, or -1 if the test fails.
 */
int test(const char *desc, test_fn test_fn, void *arg)
{
	return test_single(desc, test_fn, arg, false);
}


/**
 * @brief Executes a test function that is expected to fail.
 *
 * This function runs the provided test function and checks if it fails as expected.
 * It is useful for testing scenarios where failure is the desired outcome.
 *
 * @param desc A description of the test being executed.
 * @param test_fn The test function to execute. It must match the `test_fn` type signature.
 * @param arg An argument to pass to the test function.
 * @return 0 if the test fails as expected, or -1 if the test passes unexpectedly.
 */
int test_xf(const char *desc, test_fn test_fn, void *arg)
{
	return test_single(desc, test_fn, arg, true);
}


/**
 * @brief Executes a test function in parallel using multiple threads.
 *
 * This function runs the provided test function in parallel across the specified number of threads.
 * Each thread is assigned a unique thread ID and uses a thread-local random number generator state.
 *
 * @param desc A description of the test being executed.
 * @param test_fn The test function to execute. It must match the `test_fn` type signature.
 * @param arg An argument to pass to the test function.
 * @param thread_count The number of threads to use for parallel execution. If 0, the number of threads
 *                     will default to the number of available CPU cores.
 * @return 0 if all threads pass the test, or -1 if any thread fails.
 */
int test_parallel(const char *desc, test_fn test_fn, void *arg, unsigned thread_count)
{
	test_init(desc);

	if(!thread_count)
		thread_count = test_thread_cores_count();

	atomic_uint tid_helper;
	atomic_store_explicit(&tid_helper, 0, memory_order_relaxed);
	struct test_case t = {.fn = test_fn, .arg = arg, .tid_helper_p = &tid_helper, .rng_seed = test_random_u()};

	thr_id_t threads[thread_count];
	for(unsigned i = 0; i < thread_count; ++i)
		test_thread_start(&threads[i], test_run, &t);

	thrd_ret_t threads_ret[thread_count];
	for(unsigned i = 0; i < thread_count; ++i)
		test_thread_wait(threads[i], &threads_ret[i]);

	bool test_failed = false;
	for(unsigned i = 0; i < thread_count; ++i)
		test_failed |= threads_ret[i] == THREAD_RET_FAILURE;

	return test_fini(test_failed, false);
}


/**
 * @brief Asserts a condition and handles assertion failures.
 *
 * This function checks the provided condition. If the condition is false, it marks the current test
 * context as failed, prints the file name and line number where the assertion failed, and ensures
 * that the test cleanup function is registered to run at program exit.
 *
 * @param condition The condition to assert. If false, the assertion fails.
 * @param file_name The name of the file where the assertion is located.
 * @param line_count The line number in the file where the assertion is located.
 */
void test_assert_internal(const _Bool condition, const char *file_name, const unsigned line_count)
{
	if(condition)
		return;

	if(!atomic_flag_test_and_set_explicit(&test_setup_done, memory_order_relaxed))
		atexit(test_at_exit);

	test_ctx.assertion_failed = true;
	printf("assertion failed at line %u in file %s ", line_count, file_name);
	fflush(stdout);
}


/**
 * @brief Computes a pseudo random number in the [0, 1] range
 * @return a uniformly distributed pseudo random double value in [0, 1]
 *
 * This is the per-thread version of lcg_random()
 */
double test_random_double(void)
{
	return rng_random(&test_ctx.rng);
}


/**
 * @brief Computes a pseudo random number in the [0, n] range
 * @param n
 * @return a uniformly distributed pseudo random double value in [0, n]
 *
 * This is the per-thread version of lcg_random_range()
 */
uint64_t test_random_range(uint64_t n)
{
	return rng_random_range(&test_ctx.rng, n);
}


/**
 * @brief Computes a pseudo random 64-bit number
 * @return A uniformly distributed 64-bit pseudo random number
 *
 * This is the per-thread version of lcg_random_u()
 */
uint64_t test_random_u(void)
{
	return rng_random_u(&test_ctx.rng);
}


/**
 * @brief Retrieves the thread ID of the current parallel thread.
 *
 * This function returns the thread ID of the current thread in a parallel test execution.
 *
 * @return The thread ID of the current parallel thread.
 */
unsigned test_parallel_thread_id(void)
{
	return test_ctx.tid;
}
