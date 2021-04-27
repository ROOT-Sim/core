/**
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: CC0-1.0
 */
#include <ROOT-Sim.h>

#include <math.h>

#include <stdio.h>

#define EVT	0
#define TIMEOUT 1

#define DELAY 5

void ProcessEvent(lp_id_t me, simtime_t now, unsigned event_type,
	const void *content, unsigned size, unsigned *state)
{
	(void)me;
	(void)content;
	(void)size;
	switch (event_type) {
	case LP_INIT:
		state = malloc(sizeof(unsigned));
		*state = 0;
		SetState(state);
		simtime_t timeouttime = now + fabs(Normal() * 5.) + DELAY;
		//~ printf("[LP %lu] Init! Will timeout at %lf\n", me, timeouttime);
		ScheduleRetractableEvent(timeouttime, TIMEOUT);
		break;
	case LP_FINI:
		free(state);
		return;
	case MODEL_INIT:
	case MODEL_FINI:
		return;
	
	case TIMEOUT:
	{
		lp_id_t dest = Random() * n_lps;
		simtime_t dest_t = now + fabs(Normal() * 5.) + 20.;
		//~ printf("[LP %lu @%lf] timed out! Sending event to LP %lu @%lf\n", me, now, dest, dest_t);
		ScheduleNewEvent(dest, dest_t, EVT, NULL, 0);
		break;
	}
	case EVT:
	{
		++*state;
		//~ printf("[LP %lu @%lf] Received event! new state:%u. New timeout time:%lf\n", me, now, *state, now+DELAY);
		ScheduleRetractableEvent(now + DELAY, TIMEOUT);
		break;
	}
	}
	if (*state==101 && me==n_lps-1){
		printf("[LP %lu @%lf] DONE!\n", me, now);
	}
}

bool CanEnd(lp_id_t me, const unsigned *state)
{
	(void)me;
	return *state > 100;
}
