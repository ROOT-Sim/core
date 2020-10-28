
#include <stdlib.h>
#include <stdio.h>
#include <ROOT-Sim.h>

#include "application.h"

static unsigned max_buffers = MAX_BUFFERS;
static unsigned max_buffer_size = MAX_BUFFER_SIZE;
static unsigned complete_events = COMPLETE_EVENTS;
static bool 	new_mode = true,
		approximated = true;
static double 	tau = TAU,
		send_probability = SEND_PROBABILITY,
		alloc_probability = ALLOC_PROBABILITY,
		dealloc_probability = DEALLOC_PROBABILITY;

enum{
	OPT_MAX_BUFFERS = 128, /// this tells argp to not assign short options
	OPT_MAX_BUFFER_SIZE,
	OPT_TAU,
	OPT_SENDP,
	OPT_ALLOCP,
	OPT_DEALLOCP,
	OPT_EVT,
	OPT_MODE,
	OPT_PREC
};

const struct argp_option model_options[] = {
		{"max-buffers", 	OPT_MAX_BUFFERS, "INT", 0, NULL, 0},
		{"max-buffer-size", 	OPT_MAX_BUFFER_SIZE, "INT", 0, NULL, 0},
		{"tau", 		OPT_TAU, "DOUBLE", 0, NULL, 0},
		{"send-probability", 	OPT_SENDP, "DOUBLE", 0, NULL, 0},
		{"alloc-probability", 	OPT_ALLOCP, "DOUBLE", 0, NULL, 0},
		{"dealloc-probability", OPT_DEALLOCP, "DOUBLE", 0, NULL, 0},
		{"complete-events", 	OPT_EVT, "INT", 0, NULL, 0},
		{"old-mode", 		OPT_MODE, NULL, 0, NULL, 0},
		{"precise-mode", OPT_PREC, NULL, 0, NULL, 0},
		{0}
};

#define HANDLE_ARGP_CASE(label, fmt, var)	\
	case label: \
		if(sscanf(arg, fmt, &var) != 1){ \
			return ARGP_ERR_UNKNOWN; \
		} \
	break

static error_t model_parse(int key, char *arg, struct argp_state *state) {
	(void)state;

	switch (key) {

		HANDLE_ARGP_CASE(OPT_MAX_BUFFERS, 	"%u", 	max_buffers);
		HANDLE_ARGP_CASE(OPT_MAX_BUFFER_SIZE, 	"%u", 	max_buffer_size);
		HANDLE_ARGP_CASE(OPT_TAU, 		"%lf", 	tau);
		HANDLE_ARGP_CASE(OPT_SENDP, 		"%lf", 	send_probability);
		HANDLE_ARGP_CASE(OPT_ALLOCP, 		"%lf", 	alloc_probability);
		HANDLE_ARGP_CASE(OPT_DEALLOCP, 		"%lf", 	dealloc_probability);
		HANDLE_ARGP_CASE(OPT_EVT, 		"%u", 	complete_events);

		case OPT_MODE:
			new_mode = false;
			break;

		case OPT_PREC:
			approximated = false;
		break;

		case ARGP_KEY_SUCCESS:
			printf("\t* ROOT-Sim's PHOLD Benchmark - Current Configuration *\n");
			printf("old-mode: %s\n"
				"max-buffers: %u\n"
				"max-buffer-size: %u\n"
				"tau: %lf\n"
				"send-probability: %lf\n"
				"alloc-probability: %lf\n"
				"dealloc-probability: %lf\n"
				"approximated: %d\n",
				new_mode ? "false" : "true",
				max_buffers, max_buffer_size, tau,
				send_probability, alloc_probability, dealloc_probability, approximated);
			break;

		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

#undef HANDLE_ARGP_CASE

struct _topology_settings_t topology_settings = {.type = TOPOLOGY_OBSTACLES, .default_geometry = TOPOLOGY_GRAPH, .write_enabled = false};
struct argp model_argp = {model_options, model_parse, NULL, NULL, NULL, NULL, NULL};

<<<<<<< HEAD
=======
// These global variables are used to store execution configuration values
// They are initialised to some default values but then the initialization could change those
int	object_total_size = OBJECT_TOTAL_SIZE,
	timestamp_distribution = EXPO,
	max_size = MAX_SIZE,
	min_size = MIN_SIZE,
	num_buffers = NUM_BUFFERS,
	read_correction = NO_DISTR,
	write_correction = NO_DISTR;
unsigned int complete_alloc = COMPLETE_ALLOC;
double	write_distribution = WRITE_DISTRIBUTION,
	read_distribution = READ_DISTRIBUTION,
	tau = TAU;


void ProcessEvent(int curr_lp, simtime_t event_ts, int event_type, event_content_type *event_content, unsigned int size, void *state) {
	(void)size;

	simtime_t timestamp;
	int 	i, j,f,f_out,curr_num_buff;
	double	current_size,
		remaining_size,
		step;
	event_content_type new_event;

	lp_state_type *state_ptr = (lp_state_type*)state;
>>>>>>> origin/asym

void ProcessEvent(unsigned me, simtime_t now, int event_type, unsigned *event_content, unsigned int event_size, void *state) {
	lp_state_type *state_ptr = (lp_state_type *)state;

	switch (event_type) {

		case INIT:
			state_ptr = malloc(sizeof(lp_state_type));
                        if(state_ptr == NULL){
                                exit(-1);
                        }
                        memset(state_ptr, 0, sizeof(lp_state_type));

<<<<<<< HEAD
                        SetState(state_ptr);

			if(new_mode) {
				unsigned buffers_to_allocate = (unsigned)(Random() * max_buffers);
=======
			// Explicitly tell ROOT-Sim this is our LP's state
            SetState(state_ptr);

			timestamp = (simtime_t) (20 * Random());


			if(1 /*|| IsParameterPresent(event_content, "traditional")*/) {

				state_ptr->traditional = true;

				/*if(!IsParameterPresent(event_content, "counter")) {
					printf("Error: cannot run a traditional PHOLD benchmark if `counter` is not set!\n");
					exit(EXIT_FAILURE);
				}*/

//				state_ptr->loop_counter = GetParameterInt(event_content, "counter");
				state_ptr->events = 0;

				if(curr_lp == 0) {
					printf("Running a traditional loop-based PHOLD benchmark with counter set to %d, %d total events per LP\n", LOOP_COUNT, COMPLETE_EVENTS);
				}

				ScheduleNewEvent(curr_lp, timestamp, LOOP, NULL, 0);
			} else {

				state_ptr->traditional = false;
				state_ptr->actual_size = 0;
				state_ptr->num_elementi = 0;
				state_ptr->total_size = 0;
				state_ptr->next_lp = 0;

				// Allocate memory for counters and pointers
				state_ptr->taglie = malloc(num_buffers * sizeof(int));
				state_ptr->elementi = malloc(num_buffers * sizeof(int));
				state_ptr->head_buffs = malloc(num_buffers * sizeof(buffers *));
				state_ptr->tail_buffs = malloc(num_buffers * sizeof(buffers *));

				if(num_buffers > 1)
					step = (max_size - min_size) / (num_buffers - 1);
				else
					step = 0; // the only element will have min_size size

				current_size = min_size;
				remaining_size = object_total_size;

				// Allocate memory for buffers
				for (i = 0; i < num_buffers; i++){

					state_ptr->head_buffs[i] = NULL;
					state_ptr->tail_buffs[i] = NULL;
					state_ptr->taglie[i] = (int)current_size;


					curr_num_buff = (int)ceil(remaining_size / num_buffers / current_size);

					if(curr_num_buff == 0)
						curr_num_buff = 1;

					state_ptr->elementi[i] = curr_num_buff;

					state_ptr->num_elementi += curr_num_buff;
					state_ptr->total_size += (current_size * curr_num_buff);

					printf("[%d, %d] ", curr_num_buff, (int)current_size);

					state_ptr->head_buffs[i] = malloc(sizeof(buffers));
					state_ptr->head_buffs[i]->prev = NULL;
					state_ptr->head_buffs[i]->next = NULL;
					state_ptr->head_buffs[i]->buffer = malloc((int)current_size);
					state_ptr->tail_buffs[i] = state_ptr->head_buffs[i];

					for(j = 0; j < curr_num_buff - 1; j++) {
						buffers *tmp = malloc(sizeof(buffers));
						tmp->prev = state_ptr->tail_buffs[i];
						tmp->buffer = malloc((int)current_size);
						tmp->next = NULL;
						state_ptr->tail_buffs[i]->next = tmp;
						state_ptr->tail_buffs[i] = tmp;
					}

					remaining_size -= current_size * curr_num_buff;
					current_size += step;
>>>>>>> origin/asym

				unsigned robba_allocata = 0;
				for (unsigned i = 0; i < buffers_to_allocate; i++) {
					state_ptr->head = allocate_buffer(state_ptr->head, NULL, (unsigned)(Random() * max_buffer_size) / sizeof(unsigned));
					state_ptr->buffer_count++;
					robba_allocata += state_ptr->head->count;
				}

<<<<<<< HEAD
				while (robba_allocata < buffers_to_allocate * max_buffer_size) {
                                        state_ptr->head = allocate_buffer(state_ptr->head, NULL, (unsigned)(Random() * max_buffer_size) / sizeof(unsigned));
                                        state_ptr->buffer_count++;
                                        robba_allocata += state_ptr->head->count;
=======
				state_ptr->actual_size += current_size;


				if(curr_lp == 0) {
					ScheduleNewEvent(curr_lp, timestamp, DEALLOC, NULL, 0);
>>>>>>> origin/asym
				}
			}

			RollbackModeSet(approximated);

			ScheduleNewEvent(me, 20 * Random(), LOOP, NULL, 0);
			break;


		case LOOP:
<<<<<<< HEAD
			state_ptr->events++;
			simtime_t timestamp = now + (Expent(tau));
			ScheduleNewEvent(me, timestamp, LOOP, NULL, 0);
			if(Random() < 0.2)
				ScheduleNewEvent(FindReceiver(), timestamp, LOOP, NULL, 0);
=======
			for(i = 0; i < LOOP_COUNT; i++) {
				j = i;
			}
            f_out = RandomRange(1,3); 
            state_ptr->events++;
            timestamp = event_ts + (simtime_t)(Expent(TAU));
	    for(f = 0; f < f_out; f++){
                ScheduleNewEvent(GetReceiver(curr_lp,Random()*(DirectionsCount()+1),false), timestamp, LOOP, NULL, 0);
                timestamp = event_ts + (simtime_t)(Expent(TAU));
            }

	   if(Random() < 0.2){
		ScheduleNewEvent(FindReceiver(), timestamp, LOOP, NULL, 0);}

		break;


		case ALLOC: {

			allocation_op(state_ptr, event_content->size);

			read_op(state_ptr);
			write_op(state_ptr);

			break;
		}

		case DEALLOC: {
>>>>>>> origin/asym

			if(!new_mode){
				volatile unsigned j = 0;
				for(unsigned i = 0; i < LOOP_COUNT; i++) {
					j = i;
					(void)j;
				}
				break;
			}

<<<<<<< HEAD
			if(state_ptr->buffer_count)
				state_ptr->total_checksum ^= read_buffer(state_ptr->head, (unsigned)(Random() * state_ptr->buffer_count));

			if(state_ptr->buffer_count < max_buffers && Random() < alloc_probability) {
				state_ptr->head = allocate_buffer(state_ptr->head, NULL, (unsigned)(Random() * max_buffer_size) / sizeof(unsigned));
				state_ptr->buffer_count++;
			}

			if(state_ptr->buffer_count && Random() < dealloc_probability) {
				state_ptr->head = deallocate_buffer(state_ptr->head, (unsigned)(Random() * state_ptr->buffer_count));
				state_ptr->buffer_count--;
			}
=======
			unsigned int recv = state_ptr->next_lp;
			state_ptr->next_lp = (state_ptr->next_lp + (n_LP_tot / 4 + 1)) % n_LP_tot;
			if (recv >= n_LP_tot)
				recv = n_LP_tot - 1;

			switch (timestamp_distribution) {
				case UNIFORM: {
					timestamp = event_ts + (simtime_t)(tau * Random());
					break;
				}
				case EXPO: {
					timestamp = event_ts + (simtime_t)(Expent(tau));
					break;
				}
				default:
					timestamp = event_ts + (simtime_t)(5 * Random());
			}

			// Is there a buffer to be deallocated?
			if(fabsf(current_size + 1) < FLT_EPSILON) {
				ScheduleNewEvent(curr_lp, timestamp, DEALLOC, NULL, 0);
				break;
			} else {
>>>>>>> origin/asym

			if(state_ptr->buffer_count && Random() < send_probability) {
				unsigned i = (unsigned)(Random() * state_ptr->buffer_count);
				buffer *to_send = get_buffer(state_ptr->head, i);
				timestamp = now + (Expent(tau));

<<<<<<< HEAD
				ScheduleNewEvent(FindReceiver(), timestamp, RECEIVE, to_send->data, to_send->count * sizeof(unsigned));
=======
				ScheduleNewEvent(curr_lp, timestamp, ALLOC, &new_event, sizeof(event_content_type));
				ScheduleNewEvent(recv, timestamp, DEALLOC, NULL, 0);
>>>>>>> origin/asym

				state_ptr->head = deallocate_buffer(state_ptr->head, i);
				state_ptr->buffer_count--;
			}
			break;

		case RECEIVE:
			if(state_ptr->buffer_count >= max_buffers)
				break;
			state_ptr->head = allocate_buffer(state_ptr->head, event_content, event_size / sizeof(unsigned));
			state_ptr->buffer_count++;
			break;

		default:
			printf("[ERR] Requested to process an event neither ALLOC, nor DEALLOC, nor INIT\n");
			break;
	}
}

void RestoreApproximated(void *ptr) {
	lp_state_type *state = (lp_state_type*)ptr;
	unsigned i = state->buffer_count;
	buffer *tmp = state->head;

<<<<<<< HEAD
	while(i--) {
	    for(unsigned j = 0; j < (unsigned) tmp->count; j++) {
                tmp->data[j] = RandomRange(0, INT_MAX);
            }
            tmp = tmp->next;
	}
}

bool OnGVT(unsigned me, lp_state_type *snapshot) {
	if(snapshot->events >= complete_events){
#ifndef NDEBUG
		if(new_mode)
			printf("[LP %u] total_checksum = %u\n", me, snapshot->total_checksum);
#endif
=======
bool OnGVT(unsigned int me, lp_state_type *snapshot) {
	(void)me;
/*    if((double)snapshot->events<COMPLETE_EVENTS)
        fprintf(stdout,"PT%d: %.1f%% (%d events)\n", me, (double)snapshot->events/COMPLETE_EVENTS*100.0,snapshot->events);
        else
        fprintf(stdout,"PT%d: COMPLETE (%d events)\n", me, snapshot->events);*/  
        if(snapshot->traditional) {
		if(snapshot->events < COMPLETE_EVENTS)
			return false;
>>>>>>> origin/asym
		return true;
	}
	return false;
}

