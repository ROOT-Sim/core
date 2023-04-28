/**
 * @file test/tests/sdk/distributed.c
 *
 * @brief Test: Higher-level control messages (MPI test)
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <ROOT-Sim.h>

#include <distributed/mpi.h>

#include "control_msg.h"

struct simulation_configuration conf = {
    .lps = 16,
    .n_threads = 1,
    .serial = false,
    .dispatcher = (ProcessEvent_t)1,
    .committed = (CanEnd_t)1,
};

int main(void)
{
	RootsimInit(&conf);
	int ret = test_ctrl_msg(true);
	mpi_global_fini();
	return ret;
}
