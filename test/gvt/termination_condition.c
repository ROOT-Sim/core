/**
 * @file test/gvt/termination.c
 *
 * @brief Test: termination detection module
 *
 * SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <test.h>

#include <ROOT-Sim.h>

#include <memory.h>

static void TestConditionProcessEvent(_unused const lp_id_t me, _unused const simtime_t now, _unused const unsigned event_type,
    _unused const void *event_content, _unused const unsigned event_size, _unused void *st)
{
	if(event_type == LP_FINI)
		return;

	ScheduleNewEvent(me, now + 1.0, 0, NULL, 0);
}

static bool TestConditionCanEnd(_unused lp_id_t lid, _unused const void *state)
{
	return true;
}

static struct simulation_configuration test_condition_serial_conf = {
    .lps = 2, .dispatcher = TestConditionProcessEvent, .committed = TestConditionCanEnd, .synchronization = SERIAL};

static struct simulation_configuration test_condition_parallel_conf = {
    .lps = 2, .dispatcher = TestConditionProcessEvent, .committed = TestConditionCanEnd, .synchronization = TIME_WARP};

static int condition_termination_test(void *conf)
{
	RootsimInit((struct simulation_configuration *)conf);
	return RootsimRun();
}

int main(void)
{
	test("Test condition-based termination serial", condition_termination_test, &test_condition_serial_conf);
	test("Test condition-based termination parallel", condition_termination_test, &test_condition_parallel_conf);
}
