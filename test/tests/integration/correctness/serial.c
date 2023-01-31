/**
 * @file test/tests/integration/correctness/serial.c
 *
 * @brief Test: integration test of the serial runtime
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <test.h>

#include <tests/integration/correctness/application.h>

struct simulation_configuration conf = {
    .lps_warp = N_LPS,
    .termination_time = 0.0,
    .gvt_period = 100000,
    .log_level = LOG_SILENT,
    .stats_file = NULL,
    .ckpt_interval = 0,
    .prng_seed = 0,
    .core_binding = false,
    .serial = true,
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
	test("Correctness test (serial)", correctness, &conf);
}
