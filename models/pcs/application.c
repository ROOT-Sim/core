#include <stdlib.h>
#include <stdio.h>
#include <strings.h>

#include "application.h"

bool	approximated = true,
	fading_check = false,  // Is the model set up to periodically recompute the fading of all ongoing calls?
	variable_ta = false; // Should the call interarrival frequency change depending on the current time?
unsigned complete_calls = COMPLETE_CALLS,
	channels_per_cell = CHANNELS_PER_CELL; // Total channels per each cell
double 	ref_ta = TA,   // Initial call interarrival frequency (same for all cells)
	ta_duration = TA_DURATION, // Average duration of a call
	ta_change = TA_CHANGE; // Average time after which a call is diverted to another cell

enum {
        OPT_TA = 128, /// this tells argp to not assign short options
	OPT_TAD,
	OPT_TAC,
	OPT_CPC,
	OPT_CC,
	OPT_FR,
	OPT_VTA,
	OPT_PREC,
};

const struct argp_option model_options[] = {
		{"ta", OPT_TA, "FLOAT", 0, NULL, 0},
		{"ta-duration", OPT_TAD, "FLOAT", 0, NULL, 0},
		{"ta-change", OPT_TAC, "FLOAT", 0, NULL, 0},
		{"channels-per-cell", OPT_CPC, "UINT", 0, NULL, 0},
		{"complete-calls", OPT_CC, "INT", 0, NULL, 0},
		{"fading-recheck", OPT_FR, NULL, 0, NULL, 0},
		{"variable-ta", OPT_VTA, NULL, 0, NULL, 0},
		{"precise-mode", OPT_PREC, NULL, 0, NULL, 0},
		{0}
};

// this macro abuse looks so elegant though...
#define HANDLE_CASE(label, fmt, var)	\
	case label: \
		if(sscanf(arg, fmt, &var) != 1){ \
			return ARGP_ERR_UNKNOWN; \
		} \
	break

/*#define debug(fmt, ...) do { \
        printf("(MODEL) -> " fmt, ##__VA_ARGS__);\
        fflush(stdout);\
    } while(0)*/

#define debug(...) {}

static error_t model_parse (int key, char *arg, struct argp_state *state) {
	(void)state;

	switch (key) {
		HANDLE_CASE(OPT_TA, "%lf", ref_ta);
		HANDLE_CASE(OPT_TAD, "%lf", ta_duration);
		HANDLE_CASE(OPT_TAC, "%lf", ta_change);
		HANDLE_CASE(OPT_CPC, "%u", channels_per_cell);
		HANDLE_CASE(OPT_CC, "%u", complete_calls);

		case OPT_FR:
			fading_check = true;
			break;;
		case OPT_VTA:
			variable_ta = true;
			break;
	        case OPT_PREC:
			approximated = false;
			break;

		case ARGP_KEY_SUCCESS:
			printf("CURRENT CONFIGURATION:\ncomplete calls: %d\nTA: %f\nta_duration: %f\nta_change: %f\nchannels_per_cell: %d\nfading_recheck: %d\nvariable_ta: %d\napproximated: %d\n",
				complete_calls, ref_ta, ta_duration, ta_change, channels_per_cell, fading_check, variable_ta, approximated);
			fflush(stdout);
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

#undef HANDLE_CASE

struct argp model_argp = {model_options, model_parse, NULL, NULL, NULL, NULL, NULL};

struct _topology_settings_t topology_settings = {.default_geometry = TOPOLOGY_HEXAGON};

void ProcessEvent(unsigned int curr_lp, simtime_t event_ts, int event_type, event_content_type *event_content, unsigned int size, void *ptr) {
	(void)size;
<<<<<<< HEAD
<<<<<<< HEAD
	
	event_content_type new_event_content;
=======
=======
>>>>>>> origin/incremental

	unsigned int w;

    event_content_type new_event_content;

    bool shortcut = false;      //TRUE FOR DEBUG
>>>>>>> origin/asym

	new_event_content.cell = -1;
	new_event_content.channel = -1;
	new_event_content.call_term_time = -1;

	simtime_t handoff_time;
	simtime_t timestamp = 0;

	lp_state_type *state;
	state = (lp_state_type*)ptr;

<<<<<<< HEAD
=======
	if(state != NULL) {
		state->lvt = event_ts;
		state->executed_events++;
	}

>>>>>>> origin/asym
	switch(event_type) {

		case INIT:

			// Initialize the LP's state
			state = (lp_state_type *)malloc(sizeof(lp_state_type) + sizeof(unsigned int) * (CHANNELS_PER_CELL / BITS + 1));
			if (state == NULL){
				printf("Out of memory!\n");
				exit(EXIT_FAILURE);
			}

			SetState(state);
			bzero(state, sizeof(lp_state_type) + sizeof(unsigned int) * (CHANNELS_PER_CELL / BITS + 1));

			// Setup channel state
			state->approximated_data = malloc(sizeof(struct approximated_data_t));
			bzero(state->approximated_data, sizeof(struct approximated_data_t));

                        state->approximated_data->channel_counter = channels_per_cell;
                        state->approximated_data->channels = malloc(sizeof(channel) * channels_per_cell);

			state->ta = ref_ta;

                        RollbackModeSet(approximated);

			// Start the simulation
			timestamp = (simtime_t) (20 * Random());
            // printf("MODEL CASE <%d - INIT>: FIRST START CALL event (receiver: LP%u, type: %d, ts: %f) \n",
            //        event_type, curr_lp, START_CALL, timestamp);
			ScheduleNewEvent(curr_lp, timestamp, START_CALL, NULL, 0);

<<<<<<< HEAD
			// fading recheck
			timestamp = (simtime_t) (FADING_RECHECK_FREQUENCY * Random());
			ScheduleNewEvent(me, timestamp, FADING_RECHECK, NULL, 0);
			
=======
			// If needed, start the first fading recheck
			//if (state->fading_recheck) {
			//	timestamp = (simtime_t) (FADING_RECHECK_FREQUENCY * Random());
			//	ScheduleNewEvent(curr_lp, timestamp, FADING_RECHECK, NULL, 0);
		//	}
>>>>>>> origin/asym

			break;


		case START_CALL:
<<<<<<< HEAD

			state->approximated_data->arriving_calls++;
=======
			state->arriving_calls++;
>>>>>>> origin/asym

			if (variable_ta)
				state->ta = recompute_ta(ref_ta, now);

<<<<<<< HEAD
			// Determine the time at which a new call will be issued
			switch (DISTRIBUTION) {

				case UNIFORM:
					timestamp = now + (simtime_t)(state->ta * Random());
					break;
=======
				new_event_content.channel = allocation(state);
				new_event_content.from = curr_lp;
				new_event_content.sent_at = event_ts;

//				printf("(%d) allocation %d at %f\n", curr_lp, new_event_content.channel, event_ts);
>>>>>>> origin/asym

				case EXPONENTIAL:
					timestamp = now + (simtime_t)(Expent(state->ta));
					break;

<<<<<<< HEAD
				default:
					timestamp = now + (simtime_t) (5 * Random());

			}

			ScheduleNewEvent(me, timestamp, START_CALL, NULL, 0);
=======
					case UNIFORM:
						new_event_content.call_term_time = event_ts + (simtime_t)(ta_duration * Random());
						break;

					case EXPONENTIAL:
						new_event_content.call_term_time = event_ts + (simtime_t)(Expent(ta_duration));
						break;

					default:
 						new_event_content.call_term_time = event_ts + (simtime_t) (5 * Random() );
				}
>>>>>>> origin/asym

			if (state->approximated_data->channel_counter == 0) {
				state->approximated_data->blocked_on_setup++;
				break;
			}

			state->approximated_data->channel_counter--;

<<<<<<< HEAD
			new_event_content.channel = allocation(state);
			new_event_content.from = me;
			new_event_content.sent_at = now;

			// Determine call duration
			switch (DURATION_DISTRIBUTION) {

				case UNIFORM:
					new_event_content.call_term_time = now + (simtime_t)(ta_duration * Random());
					break;
=======
						handoff_time  = event_ts + (simtime_t)((ta_change) * Random());
						break;

					case EXPONENTIAL:
						handoff_time = event_ts + (simtime_t)(Expent(ta_change));
						break;

					default:
						handoff_time = event_ts + (simtime_t)(5 * Random());
>>>>>>> origin/asym

				case EXPONENTIAL:
					new_event_content.call_term_time = now + (simtime_t)(Expent(ta_duration));
					break;

<<<<<<< HEAD
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

<<<<<<< HEAD
=======
				if(shortcut || new_event_content.call_term_time < handoff_time) {
					ScheduleNewEvent(curr_lp, new_event_content.call_term_time, END_CALL, &new_event_content, sizeof(new_event_content));
=======
				if(1 || new_event_content.call_term_time < handoff_time) {
					ScheduleNewEvent(me, new_event_content.call_term_time, END_CALL, &new_event_content, sizeof(new_event_content));
>>>>>>> origin/power
				} else {
					new_event_content.cell = FindReceiver();
					ScheduleNewEvent(curr_lp, handoff_time, HANDOFF_LEAVE, &new_event_content, sizeof(new_event_content));
				}
			}


			if (variable_ta)
				state->ta = recompute_ta(ref_ta, event_ts);

			// Determine the time at which a new call will be issued
			switch (DISTRIBUTION) {

				case UNIFORM:
					timestamp= event_ts + (simtime_t)(state->ta * Random());
					break;

				case EXPONENTIAL:
					timestamp= event_ts + (simtime_t)(Expent(state->ta));
					break;

				default:
					timestamp= event_ts + (simtime_t) (5 * Random());

			}

			ScheduleNewEvent(curr_lp, timestamp, START_CALL, NULL, 0);
>>>>>>> origin/asym

			break;


<<<<<<< HEAD
			state->approximated_data->channel_counter++;
			state->complete_calls++ ;
			deallocation(state, event_content->channel);
=======
		case END_CALL:
			state->channel_counter++;
			state->complete_calls++;
			deallocation(curr_lp, state, event_content->channel, event_ts);
>>>>>>> origin/asym

			break;


<<<<<<< HEAD
			state->approximated_data->channel_counter++;
			state->approximated_data->leaving_handoffs++;
			deallocation(state, event_content->channel);

			new_event_content.call_term_time =  event_content->call_term_time;
			new_event_content.from = me;
			ScheduleNewEvent(event_content->cell, now, HANDOFF_RECV, &new_event_content, sizeof(new_event_content));
=======
		case HANDOFF_LEAVE:
			state->channel_counter++;
			state->leaving_handoffs++;
			deallocation(curr_lp, state, event_content->channel, event_ts);

			new_event_content.call_term_time =  event_content->call_term_time;
			new_event_content.from = curr_lp;
			ScheduleNewEvent(event_content->cell, event_ts, HANDOFF_RECV, &new_event_content, sizeof(new_event_content));
>>>>>>> origin/asym
			break;


		case HANDOFF_RECV:
<<<<<<< HEAD
			state->arriving_handoffs++;
			state->arriving_calls++;

<<<<<<< HEAD
<<<<<<< HEAD
=======
			ran = Random();
			int i;
			//i = *(event_content->dummy);
			*(event_content->dummy) = 1;
			state->dummy_flag = true;
>>>>>>> origin/ecs
=======
			state->approximated_data->arriving_handoffs++;
			state->approximated_data->arriving_calls++;
>>>>>>> origin/approximated

			if (state->approximated_data->channel_counter == 0)
				state->approximated_data->blocked_on_handoff++;
=======
			if (state->channel_counter == 0)
				state->blocked_on_handoff++;
>>>>>>> origin/asym
			else {
				state->approximated_data->channel_counter--;

				new_event_content.channel = allocation(state);
				new_event_content.call_term_time = event_content->call_term_time;


				switch (CELL_CHANGE_DISTRIBUTION) {
					case UNIFORM:
						handoff_time  = event_ts + (simtime_t)((ta_change) * Random());

						break;
					case EXPONENTIAL:
						handoff_time = event_ts + (simtime_t)(Expent(ta_change));

						break;
					default:
<<<<<<< HEAD
						handoff_time = now + (simtime_t) (5 * Random());
=======
						handoff_time = event_ts +
                                       (simtime_t) (5 * Random());
>>>>>>> origin/asym
				}

				if(new_event_content.call_term_time < handoff_time ) {
					ScheduleNewEvent(curr_lp, new_event_content.call_term_time, END_CALL, &new_event_content, sizeof(new_event_content));
				} else {
					new_event_content.cell = FindReceiver();
					ScheduleNewEvent(curr_lp, handoff_time, HANDOFF_LEAVE, &new_event_content, sizeof(new_event_content));
				}
			}


			break;

<<<<<<< HEAD
		case FADING_RECHECK:

			fading_recheck(state);
			timestamp = now + (simtime_t) (FADING_RECHECK_FREQUENCY );
			ScheduleNewEvent(me, timestamp, FADING_RECHECK, NULL, 0);
=======

		case FADING_RECHECK:

/*
			if(state->check_fading)
				state->check_fading = false;
			else
				state->check_fading = true;
*/

			fading_recheck(state);

			timestamp = event_ts + (simtime_t) (FADING_RECHECK_FREQUENCY );
			ScheduleNewEvent(curr_lp, timestamp, FADING_RECHECK, NULL, 0);

>>>>>>> origin/asym
			break;

		default:
            fprintf(stdout, "\tMODEL (PCS): UNKNOWN event type! (current LP = %u | MSG type = %d)\n", curr_lp, event_type);
			abort();

	}
}

void RestoreApproximated(void *ptr) {
	lp_state_type *state = (lp_state_type*)ptr;
	unsigned occupied = reallocate_channels(state);
	state->approximated_data->channel_counter = channels_per_cell - occupied;
}

bool OnGVT(unsigned int me, lp_state_type *snapshot) {
	(void)me;
<<<<<<< HEAD
<<<<<<< HEAD
=======
    if((double)snapshot->complete_calls/complete_calls*100.0<100.0)
        fprintf(stdout,"LP%d: %.1f%%\n", me, (double)snapshot->complete_calls/complete_calls*100.0);
   /* else
        fprintf(stdout,"LP%d: COMPLETE\n", me);*/
>>>>>>> origin/asym
=======
>>>>>>> origin/incremental

	if (snapshot->complete_calls < complete_calls)
		return false;
	return true;
}
