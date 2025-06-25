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

static _Atomic bool initialized = false;

static void DummyProcessEvent(_unused const lp_id_t me, _unused const simtime_t now, _unused const unsigned event_type,
    _unused const void *event_content, _unused const unsigned event_size, _unused void *st)
{
	if(event_type == LP_FINI)
		return;
	initialized = true;
	test_thread_sleep(20);
	ScheduleNewEvent(0, now + 1.0, 0, NULL, 0);
}

static bool DummyCanEnd(_unused lp_id_t lid, _unused const void *state)
{
	return false; // Makes the simulation run infinitely
}

static struct simulation_configuration serial_conf = {
    .lps = 1, .dispatcher = DummyProcessEvent, .committed = DummyCanEnd, .synchronization = SERIAL};

static struct simulation_configuration parallel_conf = {
    .lps = 1, .dispatcher = DummyProcessEvent, .committed = DummyCanEnd, .synchronization = TIME_WARP};

static int force_termination_test(void *conf)
{
	if(test_parallel_thread_id()) {
		while(!initialized)
			test_thread_sleep(50);
		test_thread_sleep(1000);
		RootsimStop();
		return 0;
	}
	RootsimInit((struct simulation_configuration *)conf);
	return RootsimRun();
}

int main(void)
{
	initialized = false;
	test_parallel("Test forced termination serial", force_termination_test, &serial_conf, 2);
	initialized = false;
	test_parallel("Test forced termination parallel", force_termination_test, &parallel_conf, 2);
}
