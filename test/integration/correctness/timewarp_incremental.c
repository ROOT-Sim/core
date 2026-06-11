/**
 * @file test/integration/correctness/timewarp_incremental.c
 *
 * @brief Test: correctness of the Time Warp runtime with incremental checkpointing
 *
 * Verifies that incremental checkpointing produces bit-identical LP state to a
 * reference serial execution. The model (application_incremental.c) calls
 * __write_mem() explicitly before every write to model-allocated memory,
 * exactly as a compiler instrumentation pass would inject automatically.
 * The CRC checksum computed at LP_FINI is compared against the reference output
 * from the serial run.
 *
 * SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
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
    .incremental_ckpt = true,
    .full_ckpt_period = 10,
    .core_binding = false,
    .synchronization = TIME_WARP,
    .dispatcher = ProcessEvent,
    .committed = CanEnd,
};

static int correctness(void *config)
{
	RootsimInit(config);
	return RootsimRun();
}

int main(void)
{
	crc_table_init();
	test("Correctness test (parallel, incremental checkpointing)", correctness, &conf);
}
