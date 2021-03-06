/**
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include "application.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

bool 	pcs_statistics = false,
	fading_check = false,  // Is the model set up to periodically recompute the fading of all ongoing calls?
	variable_ta = false; // Should the call interarrival frequency change depending on the current time?
unsigned complete_calls = COMPLETE_CALLS,
	channels_per_cell = CHANNELS_PER_CELL; // Total channels per each cell
double 	ref_ta = TA,   // Initial call interarrival frequency (same for all cells)
	ta_duration = TA_DURATION, // Average duration of a call
	ta_change = TA_CHANGE; // Average time after which a call is diverted to another cell

enum {
	OPT_STAT,
	OPT_TA,
	OPT_TAD,
	OPT_TAC,
	OPT_CPC,
	OPT_CC,
	OPT_FR,
	OPT_VTA,
};

struct ap_option model_options[] = {
		{"pcs-statistics", OPT_STAT, NULL, NULL},
		{"ta", OPT_TA, "FLOAT", NULL},
		{"ta-duration", OPT_TAD, "FLOAT", NULL},
		{"ta-change", OPT_TAC, "FLOAT", NULL},
		{"channels-per-cell", OPT_CPC, "UINT", NULL},
		{"complete-calls", OPT_CC, "INT", NULL},
		{"fading-recheck", OPT_FR, NULL, NULL},
		{"variable-ta", OPT_VTA, NULL, NULL},
		{0}
};

// this macro abuse looks so elegant though...
#define HANDLE_CASE(label, fmt, var)	\
	case label: \
		if(sscanf(arg, fmt, &var) != 1) { \
			printf("Bad model argument in PCS!\n"); \
			abort(); \
		} \
	break

void model_parse(int key, const char *arg) {
	
	switch (key) {
		HANDLE_CASE(OPT_TA, "%lf", ref_ta);
		HANDLE_CASE(OPT_TAD, "%lf", ta_duration);
		HANDLE_CASE(OPT_TAC, "%lf", ta_change);
		HANDLE_CASE(OPT_CPC, "%u", channels_per_cell);
		HANDLE_CASE(OPT_CC, "%u", complete_calls);

		case OPT_STAT:
			pcs_statistics = true;
			break;
		case OPT_FR:
			fading_check = true;
			break;;
		case OPT_VTA:
			variable_ta = true;
			break;

		case AP_KEY_FINI:
			printf("CURRENT CONFIGURATION:\ncomplete calls: %u\nTA: %f\nta_duration: %f\nta_change: %f\nchannels_per_cell: %u\nfading_recheck: %d\nvariable_ta: %d\n",
				complete_calls, ref_ta, ta_duration, ta_change, channels_per_cell, fading_check, variable_ta);
			fflush(stdout);
	}
}

#undef HANDLE_CASE

struct topology_settings_t topology_settings = {.default_geometry = TOPOLOGY_HEXAGON};

void ProcessEvent(lp_id_t me, simtime_t now, int event_type, event_content_type *event_content, unsigned int size, void *ptr) {
	(void)size;
	
	unsigned int w;

	//printf("%d executing %d at %f\n", me, event_type, now);

	event_content_type new_event_content;

	new_event_content.cell = -1;
	new_event_content.channel = -1;
	new_event_content.call_term_time = -1;

	simtime_t handoff_time;
	simtime_t timestamp = 0;

	lp_state_type *state;
	state = (lp_state_type*)ptr;

	if(state != NULL) {
		state->lvt = now;
		state->executed_events++;
	}


	switch(event_type) {

		case LP_INIT:

			// Initialize the LP's state
			state = (lp_state_type *)malloc(sizeof(lp_state_type));
			if (state == NULL){
				printf("Out of memory!\n");
				exit(EXIT_FAILURE);
			}

			SetState(state);

			memset(state, 0, sizeof(lp_state_type));

			state->channel_counter = channels_per_cell;
			state->ta = ref_ta;

			// Setup channel state
			state->channel_state = malloc(sizeof(unsigned int) * 2 * (CHANNELS_PER_CELL / BITS + 1));
			for (w = 0; w < state->channel_counter / (sizeof(int) * 8) + 1; w++)
				state->channel_state[w] = 0;

			// Start the simulation
			timestamp = (simtime_t) (20 * Random());
			ScheduleNewEvent(me, timestamp, START_CALL, NULL, 0);

			// If needed, start the first fading recheck
			//if (state->fading_recheck) {
				timestamp = (simtime_t) (FADING_RECHECK_FREQUENCY * Random());
				ScheduleNewEvent(me, timestamp, FADING_RECHECK, NULL, 0);
		//	}

			break;


		case START_CALL:

			state->arriving_calls++;

			if (state->channel_counter == 0) {
				state->blocked_on_setup++;
			} else {

				state->channel_counter--;

				new_event_content.channel = allocation(state);
				new_event_content.from = me;
				new_event_content.sent_at = now;

//				printf("(%d) allocation %d at %f\n", me, new_event_content.channel, now);

				// Determine call duration
				switch (DURATION_DISTRIBUTION) {

					case UNIFORM:
						new_event_content.call_term_time = now + (simtime_t)(ta_duration * Random());
						break;

					case EXPONENTIAL:
						new_event_content.call_term_time = now + (simtime_t)(Expent(ta_duration));
						break;

					default:
 						new_event_content.call_term_time = now + (simtime_t) (5 * Random() );
				}

				// Determine whether the call will be handed-off or not
				switch (CELL_CHANGE_DISTRIBUTION) {

					case UNIFORM:

						handoff_time  = now + (simtime_t)((ta_change) * Random());
						break;

					case EXPONENTIAL:
						handoff_time = now + (simtime_t)(Expent(ta_change));
						break;

					default:
						handoff_time = now + (simtime_t)(5 * Random());

				}

				if(new_event_content.call_term_time < handoff_time) {
					ScheduleNewEvent(me, new_event_content.call_term_time, END_CALL, &new_event_content, sizeof(new_event_content));
				} else {
					new_event_content.cell = FindReceiver();
					ScheduleNewEvent(me, handoff_time, HANDOFF_LEAVE, &new_event_content, sizeof(new_event_content));
				}
			}


			if (variable_ta)
				state->ta = recompute_ta(ref_ta, now);

			// Determine the time at which a new call will be issued
			switch (DISTRIBUTION) {

				case UNIFORM:
					timestamp= now + (simtime_t)(state->ta * Random());
					break;

				case EXPONENTIAL:
					timestamp= now + (simtime_t)(Expent(state->ta));
					break;

				default:
					timestamp= now + (simtime_t) (5 * Random());

			}

			ScheduleNewEvent(me, timestamp, START_CALL, NULL, 0);

			break;

		case END_CALL:

			state->channel_counter++;
			state->complete_calls++;
			deallocation(me, state, event_content->channel, now);

			break;

		case HANDOFF_LEAVE:

			state->channel_counter++;
			state->leaving_handoffs++;
			deallocation(me, state, event_content->channel, now);

			new_event_content.call_term_time =  event_content->call_term_time;
			new_event_content.from = me;
			ScheduleNewEvent(event_content->cell, now, HANDOFF_RECV, &new_event_content, sizeof(new_event_content));
			break;

		case HANDOFF_RECV:
			state->arriving_handoffs++;
			state->arriving_calls++;


			if (state->channel_counter == 0)
				state->blocked_on_handoff++;
			else {
				state->channel_counter--;

				new_event_content.channel = allocation(state);
				new_event_content.call_term_time = event_content->call_term_time;


				switch (CELL_CHANGE_DISTRIBUTION) {
					case UNIFORM:
						handoff_time  = now + (simtime_t)((ta_change) * Random());

						break;
					case EXPONENTIAL:
						handoff_time = now + (simtime_t)(Expent(ta_change));

						break;
					default:
						handoff_time = now+
						(simtime_t) (5 * Random());
				}

				if(new_event_content.call_term_time < handoff_time ) {
					ScheduleNewEvent(me, new_event_content.call_term_time, END_CALL, &new_event_content, sizeof(new_event_content));
				} else {
					new_event_content.cell = FindReceiver();
					ScheduleNewEvent(me, handoff_time, HANDOFF_LEAVE, &new_event_content, sizeof(new_event_content));
				}
			}


			break;


				case FADING_RECHECK:

/*
			if(state->check_fading)
				state->check_fading = false;
			else
				state->check_fading = true;
*/

			fading_recheck(state);

			timestamp = now + (simtime_t) (FADING_RECHECK_FREQUENCY );
			ScheduleNewEvent(me, timestamp, FADING_RECHECK, NULL, 0);

			break;

		case LP_FINI:
		case MODEL_INIT:
		case MODEL_FINI:
			break;
		default:
			fprintf(stdout, "PCS: Unknown event type! (me = %lu - event type = %d)\n", me, event_type);
			abort();

	}
}


bool CanEnd(lp_id_t me, lp_state_type *snapshot) {
	(void)me;
	
	if (snapshot->complete_calls < complete_calls)
		return false;
	return true;
}
