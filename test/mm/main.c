/**
 * @file test/tests/mm/main.c
 *
 * @brief Test: Main program of the mm test
 *
 * Entry point for the test cases related to the memory allocator
 *
 * SPDX-FileCopyrightText: 2008-2023 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <test.h>

#include <core/core.h>
#include <log/log.h>
#include <mm/distributed_mem.h>

extern int model_allocator_test(void *);
extern int model_allocator_test_hard(void *);
extern int parallel_malloc_test(void *);

int main(void)
{
	// configure the distributed memory subsystem
	global_config.lps = 1;
	global_config.bytes_per_lp = 256 * 1024;
	global_config.serial = true;

	log_init(stdout);
	distributed_mem_global_init();

	test("Testing buddy system", model_allocator_test, NULL);
	test("Testing buddy system (hard test)", model_allocator_test_hard, NULL);
	test("Testing parallel memory operations", parallel_malloc_test, NULL);

	distributed_mem_global_fini();
}
