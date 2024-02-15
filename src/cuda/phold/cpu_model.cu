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
#include <ftl/ftl.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "cpu_curand.h"
#include "settings.h"
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
#include <unistd.h>
static simtime_t lookahead = 1000;

struct simulation_configuration conf;

static int hot_phase_count = 0;

static uint get_receiver(uint me, curandState_t *cr_state, int now)
{
	int cur_hot_phase = (now / PHASE_WINDOW_SIZE);
	int hot = cur_hot_phase%HOT_PHASE_PERIOD;
	
	if(me == 0){
		if(hot == 0 && cur_hot_phase > hot_phase_count){
			hot_phase_count = cur_hot_phase;
			printf("CPU: ENTER HOT PHASE at wall clock time %f %d\n", gimme_current_time_please(), hot);
		}
		else if(hot == 1 && cur_hot_phase > hot_phase_count){
			hot_phase_count = cur_hot_phase;
			printf("CPU: ENTER COLD PHASE at wall clock time %f %d\n", gimme_current_time_please(), hot);
		}
	}
	
	if(!(hot))
		return (unsigned int) cpu_random(cr_state, HOT_FRACTION * conf.lps)/(HOT_FRACTION);
	return (unsigned int) cpu_random(cr_state, conf.lps);
}


void ProcessEvent(lp_id_t me, simtime_t now, unsigned event_type, const void *content, unsigned size,
    void *s)
{
	lp_id_t dest;
	curandState_t *state = (curandState_t *)s;
    simtime_t ts = 0;
    int incr = 0;
	(void)content;
	(void)size;
	switch(event_type) {
		case LP_INIT:
			state = (curandState_t *)rs_malloc(sizeof(curandState_t));
			if(state == NULL) abort();
            cpu_curand_init(me, 0, 0, state);
			SetState(state);

			incr =  cpu_random_exp(state, mean);
            ts =  1.0*(now + lookahead + incr);
			ScheduleNewEvent(me, ts, EVENT, NULL, 0);
			break;

		case EVENT:
//			dest =  cpu_random(state, conf.lps);
			dest =  get_receiver(me, state, (int)now);
			incr =  cpu_random_exp(state, mean);
            ts =  1.0*(now + lookahead + incr);
			if(ts < now) printf("overflow ?? %d %f now %f\n", incr, ts, now);
			ScheduleNewEvent(dest, ts, EVENT, NULL, 0);
			break;

		case LP_REINIT:
		case LP_FINI:
			break;
		default:
			fprintf(stderr, "Unknown event type\n");
			abort();
	}
}

bool CanEnd(lp_id_t me, const void *snapshot){ (void)me; (void)snapshot; return false; }

}


int main(void)
{
    conf.lps = NUM_LPS,
    conf.n_threads = NUM_THREADS,
    conf.termination_time = END_SIM_GVT,
    conf.gvt_period = GVT_PERIOD,
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
