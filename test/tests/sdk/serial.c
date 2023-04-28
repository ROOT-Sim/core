/**
* @file test/tests/sdk/distributed.c
*
* @brief Test: Higher-level control messages (sequential test)
*
* SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
* SPDX-License-Identifier: GPL-3.0-only
*/
#include <ROOT-Sim.h>

#include "control_msg.h"

struct simulation_configuration conf = {
    .lps = 16,
    .n_threads = 0,
    .serial = true,
    .dispatcher = (ProcessEvent_t)ProcessEvent,
    .committed = (CanEnd_t)1,
};

int main(void)
{
	RootsimInit(&conf);
	return test_ctrl_msg(false);
}
