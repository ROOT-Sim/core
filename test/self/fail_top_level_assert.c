/**
 * @file test/self/fail_top_level_assert.c
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

int main(void)
{
	test_assert(0);
}
