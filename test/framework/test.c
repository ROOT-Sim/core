/**
 * @file test/framework/test.c
 *
 * @brief Custom minimalistic testing framework
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <test.h>

#include <framework/thread.h>
#include <framework/rng.h>

#include <setjmp.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef _WIN32
#include <unistd.h>
#endif

struct test_ctx {
	jmp_buf jmp_buf;
	test_rng_state rng;
	unsigned tid;
	bool assertion_failed;
};

struct test_case {
	test_fn fn;
	void *arg;
	uint64_t rng_seed;
	atomic_uint *tid_helper_p;
};

static atomic_flag test_setup_done = ATOMIC_FLAG_INIT;
static atomic_uint test_success_count;
static atomic_uint test_total_count;
static _Thread_local struct test_ctx test_ctx = {.assertion_failed = false,
    .rng = (((test_rng_state)12399196U) << 64) | (test_rng_state)3532456U};

__attribute__((noreturn)) void test_fail(void)
{
	longjmp(test_ctx.jmp_buf, 1);
}

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

static void test_init(const char *desc)
{
	if(!atomic_flag_test_and_set_explicit(&test_setup_done, memory_order_relaxed))
		atexit(test_at_exit);
	atomic_fetch_add_explicit(&test_total_count, 1U, memory_order_relaxed);
	printf("%s... ", desc);
	fflush(stdout);
}

static thrd_ret_t test_run(void *args)
{
	struct test_case *t = args;

	struct test_ctx old_ctx = test_ctx;
	test_ctx.assertion_failed = false;

	if(t->tid_helper_p != NULL) {
		test_ctx.tid = atomic_fetch_add_explicit(t->tid_helper_p, 1U, memory_order_relaxed);
		rng_init(&test_ctx.rng, old_ctx.rng ^ ((__uint128_t)t->rng_seed * (test_ctx.tid + 1)));
	}

	bool has_failed;
	if(setjmp(test_ctx.jmp_buf)) {
		has_failed = true;
	} else {
		has_failed = t->fn(t->arg) != 0;
	}

	has_failed |= test_ctx.assertion_failed;
	test_ctx = old_ctx;

	return has_failed ? THREAD_RET_FAILURE : THREAD_RET_SUCCESS;
}

static int test_fini(bool has_failed, bool expected_fail)
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

static int test_single(const char *desc, test_fn test_fn, void *arg, bool should_fail)
{
	test_init(desc);

	struct test_case t = {.fn = test_fn, .arg = arg, .tid_helper_p = NULL};
	bool test_failed = test_run(&t);

	return test_fini(test_failed, should_fail);
}

int test(const char *desc, test_fn test_fn, void *arg)
{
	return test_single(desc, test_fn, arg, false);
}

int test_xf(const char *desc, test_fn test_fn, void *arg)
{
	return test_single(desc, test_fn, arg, true);
}

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

void test_assert_internal(_Bool condition, const char *file_name, unsigned line_count)
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
 * @param rng_state a test_rng_state object
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
 * @brief Computes a pseudo random 64 bit number
 * @return a uniformly distributed 64 bit pseudo random number
 *
 * This is the per-thread version of lcg_random_u()
 */
uint64_t test_random_u(void)
{
	return rng_random_u(&test_ctx.rng);
}

unsigned test_parallel_thread_id(void)
{
	return test_ctx.tid;
}
