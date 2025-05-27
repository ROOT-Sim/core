/**
 * @file test/self/unexp_pass_plain.c
 *
 * @brief Test: Test core functions of the testing framework
 *
 * This is the src that checks if assert can fail a src.
 *
 * SPDX-FileCopyrightText: 2008-2023 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <test.h>
#include <stdlib.h>

#include "stubs.h"

int main(void)
{
	test_xf("Test unexpected pass", test_want_arg_null, NULL);
}
