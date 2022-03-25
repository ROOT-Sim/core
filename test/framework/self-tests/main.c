/**
 * @file test/self-tests/main.c
 *
 * @brief Test: Main program of the self-tests tests for the testing framework
 *
 * Entry point for the test cases of the testing framework
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <stdlib.h>
#include "test.h"

#define N_THREADS 10

extern test_ret_t thread_execution(__unused void *_);
extern test_ret_t test_semaphores(__unused void *_);
extern test_ret_t test_rng(__unused void *_);

int main(void)
{
	init(N_THREADS);

	thread_execution(NULL);

	test("Testing semaphores", test_semaphores, NULL);
	parallel_test("Testing random number generator", test_rng, NULL);
	finish();
}
