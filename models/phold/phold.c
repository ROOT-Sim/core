/**
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <ROOT-Sim.h>

#include <math.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>

#define EVENT   1

struct lp_state {
	unsigned int processed;
};

struct event_content {
	unsigned long long dummy;
};

struct foo {
	unsigned bar;
//	unsigned long long foo;
	char **vettore;
	char *stringa;
};

struct conf {
	unsigned message_population;
	double timestamp_increment;
	double lookahead;
	unsigned computation_grain;
	unsigned total_events;
};

struct conf configuration = {100, 1.00, 0.50, 1000, 5000};
struct autoconf_type_map struct_conf[] = {
	{"message_population", offsetof(struct conf, message_population), AUTOCONF_UNSIGNED, NULL, 0},
	{"timestamp_increment", offsetof(struct conf, timestamp_increment), AUTOCONF_DOUBLE, NULL, 0},
	{"lookahead", offsetof(struct conf, lookahead), AUTOCONF_DOUBLE, NULL, 0},
	{"computation_grain", offsetof(struct conf, computation_grain), AUTOCONF_UNSIGNED, NULL, 0},
	{"total_events", offsetof(struct conf, total_events), AUTOCONF_UNSIGNED, NULL, 0},
	{0}
};

struct autoconf_name_map autoconf_structs[] = {{"struct conf",    struct_conf},
					       {0}};

enum {
	OPT_POP, OPT_TIMEINC, OPT_LOOKAHEAD, OPT_GRAIN, OPT_EVENTS
};

struct ap_option model_options[] = {{"population",     OPT_POP,       "UINT", NULL},
				    {"time-increment", OPT_TIMEINC,   "FLOAT", NULL},
				    {"lookahead",      OPT_LOOKAHEAD, "FLOAT", NULL},
				    {"event-grain",    OPT_GRAIN,     "UINT", NULL},
				    {"total-events",   OPT_EVENTS,    "UINT", NULL},
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
		if(me < configuration.message_population) {
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

		for(int i = 0; i < configuration.computation_grain; i++) {
			new_event.dummy += i;
		}

		simtime_t timestamp = now + configuration.lookahead + Random() * configuration.timestamp_increment;
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

	if(state->processed < configuration.total_events) {
		return false;
	}
	return true;
}
