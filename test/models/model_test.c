/**
 * @file test/models/model_test.c
 *
 * @brief The base configuration to test models
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <test.h>

#define ROOTSIM_TEST_LPS_COUNT "64"

static const char *test_arguments[] = {
	"--lp",
	ROOTSIM_TEST_LPS_COUNT,
	NULL
};

const struct test_config test_config = {
	.test_arguments = test_arguments,
};
