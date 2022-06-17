/**
 * @file test/tests/integration/integration_serial.c
 *
 * @brief Test: integration test of the serial runtime
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include "test.h"

#include "tests/integration/model/application.h"
#include "ROOT-Sim.h"

struct simulation_configuration conf = {
    .lps = N_LPS,
    .n_threads = 2,
    .termination_time = 0.0,
    .gvt_period = 100000,
    .log_level = LOG_SILENT,
    .stats_file = NULL,
    .ckpt_interval = 0,
    .prng_seed = 0,
    .core_binding = 0,
    .serial = false,
    .dispatcher = ProcessEvent,
    .committed = CanEnd,
};

static test_ret_t correctness(void *config)
{
	RootsimInit((struct simulation_configuration *)config);
	return RootsimRun();
}

int main(void)
{
	init(0);
	crc_table_init();
	test("Correctness test (parallel)", correctness, &conf);
	finish();
}
