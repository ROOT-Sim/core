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
#include <test.h>

extern test_ret_t model_allocator_test(void *);
extern test_ret_t model_allocator_test_hard(void *);
extern test_ret_t parallel_malloc_test(void *);

int main(void)
{
	init(0);

	test("Testing LP memory allocator", model_allocator_test, NULL);
	test("Testing LP memory allocator (hard test)", model_allocator_test_hard, NULL);
	test("Testing parallel memory operations", parallel_malloc_test, NULL);

	finish();
}
