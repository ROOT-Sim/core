/**
 * @file test/integration/model/application.c
 *
 * @brief Main module of the model used to verify the runtime correctness
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <integration/model/application.h>

#include <test.h>

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

struct ap_option model_options[] = {{0}};

void model_parse(int key, const char *arg)
{
	if (key == AP_KEY_FINI)
		crc_table_init();
}

#define do_random() (lcg_random(state->rng_state))

void ProcessEvent(lp_id_t me, simtime_t now, unsigned event_type, const void *event_content, unsigned event_size, void *st)
{
	lp_state *state = st;
	if (state && state->events >= COMPLETE_EVENTS) {
		if (event_type == DEINIT) {
			test_printf("%" PRIu32 "\n", state->total_checksum);
			while(state->head)
				state->head = deallocate_buffer(state->head, 0);
			free(state);
		}
		return;
	}

	if (!state && event_type != INIT) {
		abort();
	}
	switch (event_type) {
	case INIT:
		state = malloc(sizeof(lp_state));
		if (state == NULL) {
			exit(-1);
		}
		memset(state, 0, sizeof(lp_state));

		lcg_init(state->rng_state, ((test_rng_state)me + 1) *
			 4390023366657240769ULL);
		SetState(state);

		unsigned buffers_to_allocate = do_random() * MAX_BUFFERS;
		for (unsigned i = 0; i < buffers_to_allocate; ++i) {
			unsigned c = do_random() * MAX_BUFFER_SIZE / sizeof(uint64_t);
			state->head = allocate_buffer(state, NULL, c);
			state->buffer_count++;
		}

		ScheduleNewEvent(me, 20 * do_random(), LOOP, NULL, 0);
		break;

	case LOOP:
		state->events++;
		ScheduleNewEvent(me, now + do_random() * 10, LOOP, NULL, 0);
		lp_id_t dest = do_random() * n_lps;
		if(do_random() < DOUBLING_PROBABILITY && dest != me)
			ScheduleNewEvent(dest, now + do_random() * 10, LOOP, NULL, 0);

		if (state->buffer_count) {
			state->total_checksum = read_buffer(state->head, do_random() * state->buffer_count, state->total_checksum);
		}

		if (state->buffer_count < MAX_BUFFERS && do_random() < ALLOC_PROBABILITY) {
			unsigned c = do_random() * MAX_BUFFER_SIZE / sizeof(uint64_t);
			state->head = allocate_buffer(state, NULL, c);
			state->buffer_count++;
		}

		if (state->buffer_count && do_random() < DEALLOC_PROBABILITY) {
			state->head = deallocate_buffer(state->head, do_random() * state->buffer_count);
			state->buffer_count--;
		}

		if (state->buffer_count && do_random() < SEND_PROBABILITY) {
			unsigned i = do_random() * state->buffer_count;
			buffer *to_send = get_buffer(state->head, i);

			dest = do_random() * n_lps;
			ScheduleNewEvent(dest, now + do_random() * 10, RECEIVE, to_send->data, to_send->count * sizeof(uint64_t));

			state->head = deallocate_buffer(state->head, i);
			state->buffer_count--;
		}
		break;

	case RECEIVE:
		if(state->buffer_count >= MAX_BUFFERS)
			break;
		state->head = allocate_buffer(state, event_content, event_size / sizeof(uint64_t));
		state->buffer_count++;
		break;

	default:
		printf("[ERR] Requested to process an unknown event\n");
		abort();
		break;
	}
}

bool CanEnd(lp_id_t me, const void *snapshot)
{
	(void)me;
	const lp_state *state = snapshot;
	return state->events >= COMPLETE_EVENTS;
}
