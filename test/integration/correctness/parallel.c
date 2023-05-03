/**
 * @file test/tests/integration/correctness/parallel.c
 *
 * @brief Test: integration test of the serial runtime
 *
 * SPDX-FileCopyrightText: 2008-2023 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <test.h>

#include "application.h"

struct simulation_configuration conf = {
    .lps = N_LPS,
    .n_threads = 2,
    .termination_time = 0.0,
    .gvt_period = 100000,
    .log_level = LOG_SILENT,
    .stats_file = NULL,
    .ckpt_interval = 0,
    .core_binding = false,
    .serial = false,
    .dispatcher = ProcessEvent,
    .committed = CanEnd,
};

static int correctness(void *config)
{
	RootsimInit((struct simulation_configuration *)config);
	return RootsimRun();
}

int main(void)
{
	crc_table_init();
	test("Correctness test (parallel)", correctness, &conf);
}
