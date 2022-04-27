/**
* @file test/tests/core/load.c
*
* @brief Test: initialization of the core library
*
* SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
* SPDX-License-Identifier: GPL-3.0-only
*/
#include "test.h"

#include <stdio.h>

#include "ROOT-Sim.h"

void DummProcessEvent(lp_id_t me, simtime_t now, unsigned event_type, const void *event_content,
					  unsigned event_size, void *st)
{
	(void)me;
	(void)now;
	(void)event_type;
	(void)event_content;
	(void)event_size;
	(void)st;
}

bool DummyCanEnd(lp_id_t lid, const void *state)
{
	(void)lid;
	(void)state;
	return false;
}

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
	conf.dispatcher = DummProcessEvent;
	test_xf("CanEnd not set", init_rootsim, &conf);
	conf.committed = DummyCanEnd;
	test("Initialization", init_rootsim, &conf);

	finish();
}
