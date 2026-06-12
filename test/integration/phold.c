/**
 * @file test/integration/phold.c
 *
 * @brief A simple and stripped phold implementation
 *
 * SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <test.h>

#include <ROOT-Sim.h>

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "test_rng.h"

#ifndef NUM_LPS
#define NUM_LPS 8192
#endif

#ifndef NUM_THREADS
#define NUM_THREADS 0
#endif

#define EVENT 1


struct phold_message {
	long int dummy_data;
};

static simtime_t p_remote = 0.25;
static simtime_t mean = 1.0;
static simtime_t lookahead = 0.0;
static int start_events = 1;



static double Expent(struct test_rng_state *state)
{
	return mean * (-log(1. - test_rng_random(state)));
}


void ProcessEvent(const lp_id_t me, const simtime_t now, const unsigned event_type, _unused const void *content,
	_unused const unsigned size, void *s)
{
	const struct phold_message new_event = {0};
	lp_id_t dest;
	struct test_rng_state *state = s;

	switch(event_type) {
		case LP_INIT:
			state = rs_malloc(sizeof(*state));
			if(state == NULL)
				abort();
			test_rng_set_seed(me, state);
			SetState(state);

			for(int i = 0; i < start_events; i++)
				ScheduleNewEvent(me, Expent(state) + lookahead, EVENT, &new_event, sizeof(new_event));
			break;

		case LP_FINI:
			break;

		case EVENT:
			dest = me;
			if(test_rng_random(state) <= p_remote)
				dest = (lp_id_t)(test_rng_random(state) * NUM_LPS);

			ScheduleNewEvent(dest, now + Expent(state) + lookahead, EVENT, &new_event, sizeof(new_event));
			break;

		default:
			fprintf(stderr, "Unknown event type\n");
			abort();
	}
}

bool CanEnd(_unused lp_id_t me, _unused const void *snapshot)
{
	return false;
}

struct simulation_configuration conf = {
    .lps = NUM_LPS,
    .n_threads = NUM_THREADS,
    .termination_time = 1000,
    .gvt_period = 1000,
    .log_level = LOG_INFO,
    .stats_file = "phold",
    .ckpt_interval = 0,
    .core_binding = true,
    .synchronization = TIME_WARP,
    .dispatcher = ProcessEvent,
    .committed = CanEnd,
};

int main(void)
{
	RootsimInit(&conf);
	return RootsimRun();
}
