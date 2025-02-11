/**
 * @file test/tests/core/load.c
 *
 * @brief Test: initialization of the core library
 *
 * SPDX-FileCopyrightText: 2008-2023 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <test.h>

#include <ROOT-Sim.h>

#include <memory.h>

static void DummyProcessEvent(_unused lp_id_t me, _unused simtime_t now, _unused unsigned event_type,
    _unused const void *event_content, _unused unsigned event_size, _unused void *st)
{}

static bool DummyCanEnd(_unused lp_id_t lid, _unused const void *state)
{
	return false;
}

static struct simulation_configuration conf = {
    .lps = 0,
    .dispatcher = NULL,
    .committed = NULL,
};

static const struct simulation_configuration valid_conf = {
    .lps = 1,
    .dispatcher = DummyProcessEvent,
    .committed = DummyCanEnd,
};

static int run_rootsim(_unused void *_)
{
	return RootsimRun();
}

static int init_rootsim(void *config)
{
	return RootsimInit((struct simulation_configuration *)config);
}

int main(void)
{
	test_xf("Start simulation with no configuration", run_rootsim, NULL);

	memcpy(&conf, &valid_conf, sizeof(conf));
	conf.lps = 0;
	test_xf("LPs not set", init_rootsim, &conf);
	test_xf("Start simulation with no LPs", run_rootsim, NULL);

	memcpy(&conf, &valid_conf, sizeof(conf));
	conf.dispatcher = NULL;
	test_xf("Handler not set", init_rootsim, &conf);
	test_xf("Start simulation with no handler", run_rootsim, NULL);

	memcpy(&conf, &valid_conf, sizeof(conf));
	conf.committed = NULL;
	test_xf("CanEnd not set", init_rootsim, &conf);
	test_xf("Start simulation with no CanEnd", run_rootsim, NULL);

	memcpy(&conf, &valid_conf, sizeof(conf));
	test("Initialization", init_rootsim, &conf);
	test("Dummy simulation", run_rootsim, NULL);
}
