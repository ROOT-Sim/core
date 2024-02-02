/**
 * @file test/tests/integration/phold.c
 *
 * @brief A simple and stripped phold implementation
 *
 * SPDX-FileCopyrightText: 2008-2023 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
extern "C" {

#include <ROOT-Sim.h>

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "cpu_curand.h"
}


#ifndef NUM_LPS
#define NUM_LPS 8192
#endif

#ifndef NUM_THREADS
#define NUM_THREADS 0
#endif

#define EVENT 1

unsigned int mean = 10000;

extern "C" {
static simtime_t lookahead = 0.0;

struct simulation_configuration conf;

void ProcessEvent(lp_id_t me, simtime_t now, unsigned event_type, const void *content, unsigned size,
    void *s)
{
	lp_id_t dest;
	curandState_t *state = (curandState_t *)s;
    unsigned int ts = 0;
    
	switch(event_type) {
		case LP_INIT:
			state = (curandState_t *)rs_malloc(sizeof(curandState_t));
			if(state == NULL) abort();
            cpu_curand_init(me, 0, 0, state);
			SetState(state);

            ts =  now + lookahead + cpu_random_exp(state, mean);
			ScheduleNewEvent(me, ts, EVENT, NULL, 0);
			break;

		case EVENT:
		case LP_REINIT:
			dest = cpu_random(state, conf.lps);
            ts =  now + lookahead + cpu_random_exp(state, mean);
			ScheduleNewEvent(dest, ts, EVENT, NULL, 0);
			break;

		case LP_FINI:
			break;
		default:
			fprintf(stderr, "Unknown event type\n");
			abort();
	}
}

bool CanEnd(lp_id_t me, const void *snapshot){ return false; }

}


int main(void)
{
    conf.lps = NUM_LPS,
    conf.n_threads = NUM_THREADS,
    conf.termination_time = 500000000,
    conf.gvt_period = 1000*250,
    conf.log_level = LOG_INFO,
    conf.stats_file = "phold",
    conf.ckpt_interval = 0,
    conf.core_binding = true,
    conf.serial = false,
    conf.use_gpu = true,
    conf.dispatcher = ProcessEvent,
    conf.committed = CanEnd,
	RootsimInit(&conf);
	return RootsimRun();
}

