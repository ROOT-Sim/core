/**
 * @file test/tests/integration/phold.c
 *
 * @brief A simple and stripped phold implementation
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <test.h>

#include <ROOT-Sim.h>

#include <stdio.h>

#ifndef NUM_LPS
#define NUM_LPS 8192
#endif

#ifndef NUM_THREADS
#define NUM_THREADS 4
#endif

#define EVENT 1

struct phold_message {
	long int dummy_data;
};

static simtime_t p_remote = 0.35;
static simtime_t mean = 1.0;
static simtime_t lookahead = 0.0;
static int start_events = 1;

void ProcessEvent(lp_id_t me, simtime_t now, unsigned event_type, _unused const void *content, _unused unsigned size,
    _unused void *s)
{
	struct phold_message new_event = {0};
	lp_id_t dest;

	switch(event_type) {
		case LP_FINI:
			break;

		case LP_INIT:
			for(int i = 0; i < start_events; i++)
				ScheduleNewEvent(me, Expent(mean) + lookahead, EVENT, &new_event, sizeof(new_event));
			break;

		case EVENT:
			dest = 0;
			if(Random() <= p_remote)
				dest = (lp_id_t)(Random() * NUM_LPS);

			ScheduleNewEvent(dest, now + Expent(mean) + lookahead, EVENT, &new_event, sizeof(new_event));
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
    .lps_racer = NUM_LPS,
    .n_threads_racer = NUM_THREADS,
    .termination_time = 1000,
    .gvt_period = 100000,
    .log_level = LOG_INFO,
    .stats_file = "phold",
    .ckpt_interval = 0,
    .prng_seed = 0,
    .core_binding = true,
    .serial = false,
    .dispatcher = ProcessEvent,
    .committed = CanEnd,
};

int main(void)
{
	RootsimInit(&conf);
	return RootsimRun();
}
