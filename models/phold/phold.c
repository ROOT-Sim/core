/**
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <ROOT-Sim.h>

#include <math.h>
#include <string.h>
#include <stdio.h>

// Model configuration variables
#define MESSAGE_POPULATION     100
#define TIMESTAMP_INCREMENT    1.00
#define LOOKAHEAD              0.50
#define COMPUTATION_GRAIN      100000
#define TOTAL_EVENTS           50000

#define EVENT   1

struct lp_state {
	long long seed;
	double lvt;
	unsigned int processed;
};

struct event_content {
	unsigned long long dummy;
};

void ProcessEvent(lp_id_t me, simtime_t now, unsigned event_type, const void *content, unsigned size, unsigned *s)
{

	struct lp_state *state = (struct lp_state *) s;
	struct event_content new_event = {me};

	unsigned int receiver;
	double timestamp;

	if(state != NULL) {
		state->lvt = now;
	}

	switch(event_type) {
		case LP_INIT:
			state = malloc(sizeof(unsigned));
			memset(state, 0, sizeof(struct lp_state));
			SetState(state);

			// Inject events in the system
			if(me < MESSAGE_POPULATION) {
				ScheduleNewEvent(me, 0.01, EVENT, NULL, 0);
			}
			break;

		case LP_FINI:
			free(state);
			return;

		case MODEL_INIT:
		case MODEL_FINI:
			return;

		case EVENT:
			state->processed++;

			for(int i = 0; i < COMPUTATION_GRAIN; i++) {
				new_event.dummy += i;
			}

			receiver = (unsigned int) (Random() * (double) n_lps);
			timestamp = now + LOOKAHEAD + Random() * TIMESTAMP_INCREMENT;
			ScheduleNewEvent(receiver, timestamp, EVENT, &new_event, sizeof(struct event_content));

			break;

		default:
			fprintf(stderr, "Unknown event type\n");
			abort();
	}
}

bool CanEnd(lp_id_t me, const unsigned *snapshot)
{
	struct lp_state *state = (struct lp_state *) snapshot;

	if(state->processed < TOTAL_EVENTS) {
		return false;
	}
	return true;
}
