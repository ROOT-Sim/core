/**
 * @file test/self/fail_implicit_multi.c
 *
 * @brief Test: Test core functions of the testing framework
 *
 * This is the src that checks if explicit fail works.
 *
 * SPDX-FileCopyrightText: 2008-2023 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <test.h>
#include "stubs.h"

#include <stdatomic.h>

static int test_fail_once(_unused void *arg)
{
	static atomic_flag failed = ATOMIC_FLAG_INIT;
	return !atomic_flag_test_and_set_explicit(&failed, memory_order_relaxed);
}

int main(void)
{
	test_parallel("Test multithreaded fail", test_fail_once, (void *)1, 0);
}
