/**
 * @file test/integration/integration_parallel_single.c
 *
 * @brief Test: integration test of the parallel runtime with one single thread
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <test.h>

#include <integration/model/application.h>

static const char *test_arguments[] = {
	"--lp",
	"64",
	"--wt",
	"1",
	NULL
};

const struct test_config test_config = {
	.test_arguments = test_arguments,
	.expected_output = model_expected_output_64,
	.expected_output_size = sizeof(model_expected_output_64)
};
