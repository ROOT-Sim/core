/**
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <ROOT-Sim.h>

#include <math.h>
#include <string.h>
#include <stdio.h>

#define EVENT   1

struct lp_state {
	unsigned int processed;
};

struct event_content {
	unsigned long long dummy;
};

// Default configuration values
unsigned int message_population = 100;
double timestamp_increment = 1.00;
double lookahead = 0.50;
unsigned int computation_grain = 1000;
unsigned int total_events = 5000;

enum {
	OPT_POP, OPT_TIMEINC, OPT_LOOKAHEAD, OPT_GRAIN, OPT_EVENTS
};

struct ap_option model_options[] = {{"population",     OPT_POP,       "UINT",   NULL},
				    {"time-increment", OPT_TIMEINC,   "FLOAT", NULL},
				    {"lookahead",      OPT_LOOKAHEAD, "FLOAT", NULL},
				    {"event-grain",    OPT_GRAIN,     "UINT",  NULL},
				    {"total-events",   OPT_EVENTS,    "UINT",   NULL},
				    {0}};

struct topology_t *topology;

__attribute__((used))
void ProcessEvent(lp_id_t me, simtime_t now, unsigned event_type, const void *content, unsigned size, void *s)
{

	struct lp_state *state = (struct lp_state *) s;
	struct event_content new_event = {me};

	switch(event_type) {

	case MODEL_INIT:
		topology = TopologyInit(TOPOLOGY_MESH, 0);
		break;

	case LP_INIT:
		state = malloc(sizeof(struct lp_state));
		memset(state, 0, sizeof(*state));
		SetState(state);

		// Inject events in the system
		if(me < message_population) {
			ScheduleNewEvent(me, 0.01, EVENT, NULL, 0);
		}
		break;

	case LP_FINI:
		free(state);
		return;

		case MODEL_FINI:
			return;

	case EVENT:
		state->processed++;

		for(int i = 0; i < computation_grain; i++) {
			new_event.dummy += i;
		}

		simtime_t timestamp = now + lookahead + Random() * timestamp_increment;
		ScheduleNewEvent(FindReceiver(topology), timestamp, EVENT, &new_event, sizeof(struct event_content));

		break;

	default:
		fprintf(stderr, "Unknown event type\n");
		abort();
	}
}

__attribute__((used))
bool CanEnd(lp_id_t me, const void *snapshot)
{
	struct lp_state *state = (struct lp_state *) snapshot;

	if(state->processed < total_events) {
		return false;
	}
	return true;
}
