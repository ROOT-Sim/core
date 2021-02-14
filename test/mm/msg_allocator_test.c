/**
 * @file test/buddy/buddy_test.c
 *
 * @brief Test: message allocator
 *
 * A test of the message allocator
 * @todo This is still just a stub, implement the actual test!
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <test.h>
#include <mm/msg_allocator.h>

static int msg_allocator_test(void)
{
	msg_allocator_init();
	// TODO ACTUAL TEST
	msg_allocator_fini();
	return 0;
}

const struct test_config test_config = {
	.threads_count = 4,
	.test_fnc = msg_allocator_test
};
