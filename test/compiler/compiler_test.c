/**
 * @file test/compiler/compiler_test.c
 *
 * @brief Test: compiler wrapper
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <test.h>

// FIXME this test is totally dummy and useless

static const char *test_arguments[] = {
	"--version",
	NULL
};

const struct test_config test_config = {
	.test_arguments = test_arguments
};
