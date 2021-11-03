/**
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <ROOT-Sim.h>

#include "pcs.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

bool preload_model = false;
unsigned complete_calls = COMPLETE_CALLS, channels_per_cell = CHANNELS_PER_CELL; // Total channels per each cell
double ref_ta = TA_BASE,   // Initial call inter-arrival frequency (reference for all cells)
ta_duration = TA_DURATION, // Average duration of a call
ta_change = TA_CHANGE; // Average time after which a call is diverted to another cell

enum {
	OPT_TAREF, OPT_TAD, OPT_TAC, OPT_CPC, OPT_CC, OPT_NOPRELOAD
};

struct ap_option model_options[] = {{"ta-base",           OPT_TAREF, "FLOAT",  NULL},
				    {"ta-duration",       OPT_TAD,   "FLOAT",  NULL},
				    {"ta-change",         OPT_TAC,   "FLOAT",  NULL},
				    {"channels-per-cell", OPT_CPC,   "UINT",   NULL},
				    {"complete-calls",    OPT_CC,    "INT",    NULL},
				    {"no-preload-model",  OPT_NOPRELOAD, NULL, NULL},
				    {0}};

// this macro abuse looks so elegant though...
#define HANDLE_CASE(label, fmt, var)        \
        case label: \
                if(sscanf(arg, fmt, &var) != 1) { \
                        printf("Bad model argument in PCS!\n"); \
                        abort(); \
                } \
        break

void model_parse(int key, const char *arg)
{

	switch(key) {
		HANDLE_CASE(OPT_TAREF, "%lf", ref_ta);
		HANDLE_CASE(OPT_TAD, "%lf", ta_duration);
		HANDLE_CASE(OPT_TAC, "%lf", ta_change);
		HANDLE_CASE(OPT_CPC, "%u", channels_per_cell);
		HANDLE_CASE(OPT_CC, "%u", complete_calls);

		case OPT_NOPRELOAD:
			preload_model = false;
			break;

		case AP_KEY_FINI:
			printf("CURRENT CONFIGURATION:\ncomplete calls: %u\nreference TA: %.02f\nta_duration: %.02f\n"
	  			"ta_change: %.02f\nchannels_per_cell: %u\npreload_model: %s\n",
			       complete_calls, ref_ta, ta_duration, ta_change, channels_per_cell,
			       preload_model ? "true" : "false");
			fflush(stdout);
	}
}

#undef HANDLE_CASE

struct topology_t *topology;

double current_ta(struct lp_state *state, double current_time)
{
	int now = (int) current_time;
	now %= WEEK;

	// Weekend
	if(now > 5 * DAY) {
		return state->ta * WEEKEND_FACTOR;
	}

	now %= DAY;
	if(now < EARLY_MORNING) {
		return state->ta * EARLY_MORNING_FACTOR;
	}
	if(now < MORNING) {
		return state->ta * MORNING_FACTOR;
	}
	if(now < LUNCH) {
		return state->ta * LUNCH_FACTOR;
	}
	if(now < AFTERNOON) {
		return state->ta * AFTERNOON_FACTOR;
	}
	if(now < EVENING) {
		return state->ta * EVENING_FACTOR;
	}
	// night
	return state->ta * NIGHT_FACTOR;
}

void deallocation(struct lp_state *state, int channel)
{
	struct channel **c = &state->channels.next;
	struct channel *remove = NULL;

	while(*c && (*c)->channel_id != channel)
		c = &(*c)->next;

	if(*c != NULL) {
		remove = *c;
		*c = (*c)->next;
		free(remove);

		state->channel_counter++;
		RESET_CHANNEL(state->channel_state, channel);
	} else {
		fprintf(stderr, "Trying to deallocate a non-allocated channel!\n");
		abort();
	}
}

static inline double compute_gain(double gain, struct lp_state *state)
{
	return gain * pow((double) 10.0, (Random()));
}


static int allocation(struct lp_state *state)
{
	int index = -1;

	if(state->channel_counter == 0) {
		return -1;
	}

	for(int i = 0; i < CHANNELS_PER_CELL; i++) {
		if(!CHECK_CHANNEL(state->channel_state, i)) {
			index = i;
			break;
		}
	}

	if(index != -1) {
		// Mark the channel as used
		SET_CHANNEL(state->channel_state, index);
		state->channel_counter--;

		// Allocate metadata record
		struct channel *c = malloc(sizeof(*c));
		c->channel_id = index;
		c->fading = Expent(1.0);

		// Compute accurate values for the call record
		// Accurate simulation based on the model in:
		// S. Kandukuri and S. Boyd,
		// "Optimal power control in interference-limited fading wireless channels with outage-probability specifications,"
		// in IEEE Transactions on Wireless Communications, vol. 1, no. 1, pp. 46-55, Jan. 2002, doi: 10.1109/7693.975444.
		struct channel *curr = state->channels.next;
		double summation = 0.0;
		while(curr != NULL) {
			curr->fading = Expent(1.0);
			summation += compute_gain(CROSS_PATH_GAIN, state) * curr->power * curr->fading;
			curr = curr->next;
		}

		if(summation == 0.0) {
			c->power = MIN_POWER;
		} else {
			c->power = ((SIR_AIM * summation) / (compute_gain(PATH_GAIN, state) * c->fading));
			c->power = c->power < MIN_POWER ? MIN_POWER : c->power;
			c->power = c->power > MAX_POWER ? MAX_POWER : c->power;
		}

		// Link the new record
		c->next = state->channels.next;
		state->channels.next = c;
	} else {
		fprintf(stderr, "Model error\n");
		exit(EXIT_FAILURE);
	}

	return index;
}

static void setup_call(unsigned me, double now, struct lp_state *state, double end_time)
{
	double timestamp;
	double handoff_time;

	struct event_content new_call = {me, -1, -1};
	new_call.channel = allocation(state);

	if(end_time < 0) {
		new_call.end_time = now + Expent(ta_duration);

		timestamp = now + Expent(current_ta(state, now));
		ScheduleNewEvent(me, timestamp, START_CALL, NULL, 0);
	} else {
		new_call.end_time = end_time;
	}

	if(new_call.channel == -1) {
		state->blocked_on_setup++;
		return;
	}

	handoff_time = now + Expent(ta_change);

	if(new_call.end_time <= handoff_time) {
		ScheduleNewEvent(me, new_call.end_time, END_CALL, &new_call, sizeof(new_call));
	} else {
		new_call.cell = FindReceiver(topology);
		ScheduleNewEvent(me, handoff_time, HANDOFF_LEAVE, &new_call, sizeof(new_call));
	}
}

static void preload(unsigned int me, struct lp_state *state)
{
	struct event_content new_call = {me, -1, -1};

	double eta = TA_DURATION / (CHANNELS_PER_CELL * state->ta);
	eta = eta > 1.0 ? 1.0 : eta;

	for(int i = 0; i < CHANNELS_PER_CELL * eta; i++) {
		new_call.channel = allocation(state);
		new_call.end_time = Expent(ta_duration);
		double handoff_time = Expent(ta_change);

		if(new_call.end_time <= handoff_time) {
			ScheduleNewEvent(me, new_call.end_time, END_CALL, &new_call, sizeof(new_call));
		} else {
			new_call.cell = FindReceiver(topology);
			ScheduleNewEvent(me, handoff_time, HANDOFF_LEAVE, &new_call, sizeof(new_call));
		}
	}
}

__attribute__((used))
void ProcessEvent(lp_id_t me, simtime_t now, int event_type, void *e, unsigned int size, void *s)
{
	(void) size;
	struct lp_state *state = (struct lp_state *) s;
	struct event_content *evt = (struct event_content *) e;

	if(state != NULL) {
		state->lvt = now;
	}

	switch(event_type) {

		case MODEL_INIT:
			topology = TopologyInit(TOPOLOGY_HEXAGON, 0);
			break;

		case LP_INIT:
			state = malloc(sizeof(struct lp_state));
			if(state == NULL) {
				fprintf(stderr, "Out of memory!\n");
				exit(EXIT_FAILURE);
			}
			SetState(state);
			memset(state, 0, sizeof(struct lp_state));

			// Setup channel state
			state->channel_state = malloc(sizeof(unsigned long long) * (channels_per_cell / BITS + 1));
			memset(state->channel_state, 0, sizeof(unsigned long long) * (channels_per_cell / BITS + 1));

			state->channel_counter = channels_per_cell;
			state->channels.channel_id = -1;
			state->ta = TA_BASE; // <--- configure this on a per-LP basis based on a conf file?

			if(preload_model) {
				preload(me, state);
			}

			// Start the simulation
			ScheduleNewEvent(me, now + Random(), START_CALL, NULL, 0);

			break;

		case START_CALL:
			state->arriving_calls++;
			setup_call(me, now, state, -1);
			break;

		case END_CALL:
			state->complete_calls++;
			deallocation(state, evt->channel);
			break;

		case HANDOFF_LEAVE:
			state->handoff_leaving++;
			deallocation(state, evt->channel);
			ScheduleNewEvent(evt->cell, now, HANDOFF_RECV, evt, sizeof(*evt));
			break;

		case HANDOFF_RECV:
			state->arriving_calls++;
			state->handoff_entering++;
			setup_call(me, now, state, evt->end_time);
			break;

		case LP_FINI:
			free(state);
			return;

		case MODEL_FINI:
			return;
		default:
			fprintf(stdout, "PCS: Unknown event type! (me = %lu - event type = %d)\n", me, event_type);
			abort();

	}
}

__attribute__((used))
bool CanEnd(lp_id_t me, struct lp_state *snapshot)
{
	(void) me;

	if(snapshot->complete_calls < complete_calls) {
		return false;
	}
	return true;
}
