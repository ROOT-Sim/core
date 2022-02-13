/**
* @file test/core/load.c
*
* @brief Test: initialization of the core library
*
* SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
* SPDX-License-Identifier: GPL-3.0-only
*/
#include <test.h>

#include <stdlib.h>
#include <stdio.h>

#include <ROOT-Sim.h>


void foo() {}

struct simulation_configuration conf = {
    .lps = 0,
    .dispatcher = NULL,
    .committed = NULL,
};

static test_ret_t run_rootsim(__unused void *_)
{
	return RootsimRun();
}

static test_ret_t init_rootsim(void *config)
{
	return RootsimInit((struct simulation_configuration *)config);
}

int main(void)
{
	init(0);

	test_xf("Start simulation with no configuration", run_rootsim, NULL);
	test_xf("LPs not set", init_rootsim, &conf);
	conf.lps = 1;
	test_xf("Handler not set", init_rootsim, &conf);
	conf.dispatcher = (ProcessEvent_t)foo;
	test_xf("CanEnd not set", init_rootsim, &conf);
	conf.committed = (CanEnd_t)foo;
	test("Initialization", init_rootsim, &conf);

	finish();
}
