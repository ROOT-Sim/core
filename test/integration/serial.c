/**
 * @file test/integration/integration_serial.c
 *
 * @brief Test: integration test of the serial runtime
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <test.h>
#include <integration/model/application.h>
#include <ROOT-Sim.h>

struct simulation_configuration conf = {
    .lps = N_LPS,
    .n_threads = 4,
    .termination_time = 0.0,
    .gvt_period = 1000,
    .log_level = LOG_SILENT,
    .stats_file = NULL,
    .ckpt_interval = 10,
    .prng_seed = 0,
    .core_binding = 0,
    .serial = true,
    .dispatcher = ProcessEvent,
    .committed = CanEnd,
};

int main(void)
{
	init();

	crc_table_init();

	RootsimInit(&conf);
	test("Correctness test (serial)", RootsimRun);

	finish();
}
