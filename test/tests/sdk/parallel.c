/**
* @file test/tests/sdk/parallel.c
*
* @brief Test: Higher-level control messages (parallel test)
*
* SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
* SPDX-License-Identifier: GPL-3.0-only
*/
#include <ROOT-Sim.h>

#include "control_msg.h"

struct simulation_configuration conf = {
    .lps = 16,
    .n_threads = 0,
    .serial = false,
    .dispatcher = (ProcessEvent_t)1,
    .committed = (CanEnd_t)1,
};

int main(void)
{
	RootsimInit(&conf);
	return test_ctrl_msg(false);
}
