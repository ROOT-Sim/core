/**
* @file test/tests/mm/main.c
*
* @brief Test: Main program of the mm test
*
* Entry point for the test cases related to the memory allocator
*
* SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
* SPDX-License-Identifier: GPL-3.0-only
*/
#include <stdlib.h>
#include "test.h"
#include "log/log.h"

#define N_THREADS 4

extern test_ret_t model_allocator_test(void *);
extern test_ret_t model_allocator_test_hard(void *);
extern test_ret_t parallel_malloc_test(void *);

int main(void)
{
	init(N_THREADS);

	// This is necessary to print out of memory messages without crashing the test
	log_init(tmpfile());

	test("Testing buddy system", model_allocator_test, NULL);
	test("Testing buddy system (hard test)", model_allocator_test_hard, NULL);
	parallel_test("Testing parallel memory operations", parallel_malloc_test, NULL);

	finish();
}
