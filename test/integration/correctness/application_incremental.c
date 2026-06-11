/**
 * @file test/integration/correctness/application_incremental.c
 *
 * @brief Instrumented model for incremental checkpointing correctness tests
 *
 * This file provides the same model logic as application.c + functions.c combined,
 * but with explicit __write_mem() calls before every write to model-allocated memory.
 * This simulates the effect of compiler-level software instrumentation (which would
 * normally be injected by a source-to-source transformation pass before every store
 * to model-allocated memory).
 *
 * The WRMEM(lval) macro encapsulates the pattern: call __write_mem() with the address
 * and size of the destination, then perform the assignment. For bulk writes (memcpy,
 * memset, loops), __write_mem() is called with the full region before the operation.
 *
 * SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include "application.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * __write_mem() is declared in mm/checkpoint/incremental.c and linked from rscore.
 * It is normally injected by the instrumentation pass; here we call it explicitly.
 */
extern void __write_mem(const void *ptr, size_t size);

/**
 * WRMEM(lval) — instrument a scalar write.
 * Usage: WRMEM(ptr->field) = value;
 * Expands to: notify dirty-tracking, then yield lval as an lvalue.
 */
#define WRMEM(lval) (*(__write_mem(&(lval), sizeof(lval)), &(lval)))

/**
 * WRMEM_BUF(ptr, size) — instrument a bulk write (memcpy/memset/loop).
 * Call once before performing any bulk write to model-allocated memory.
 */
#define WRMEM_BUF(ptr, size) (__write_mem((ptr), (size)))

/* -----------------------------------------------------------------------
 * Instrumented helper functions. Replaces functions.c for this test.
 * A refactor may probably be required here, but I am now happy with this.
 * -------------------------------------------------------------------- */

uint32_t crc_update(const uint64_t *buf, size_t n, uint32_t crc);

buffer *get_buffer(buffer *head, unsigned i)
{
	while(i--)
		head = head->next;
	return head;
}

uint32_t read_buffer(buffer *head, const unsigned i, const uint32_t old_crc)
{
	head = get_buffer(head, i);
	return crc_update(head->data, head->count, old_crc);
}

buffer *allocate_buffer(lp_state *state, const unsigned *data, const unsigned count)
{
	buffer *buf = rs_malloc(sizeof(buffer) + count * sizeof(uint64_t));

	/*
	 * buf->next, buf->count, and buf->data[] are freshly allocated memory.
	 * They will be tracked automatically if instrumentation marks the entire
	 * allocated block; but since we are explicit, call __write_mem for each
	 * field we write before writing it.
	 */
	WRMEM(buf->next) = state->head;
	WRMEM(buf->count) = count;

	if(data != NULL) {
		WRMEM_BUF(buf->data, count * sizeof(uint64_t));
		memcpy(buf->data, data, count * sizeof(uint64_t));
	} else {
		for(unsigned i = 0; i < count; i++) {
			/*
			 * rng_random_u modifies state->rng_state in place —
			 * that write is to model-allocated memory too.
			 */
			WRMEM(state->rng_state);
			WRMEM(buf->data[i]) = rng_random_u(&state->rng_state);
		}
	}

	return buf;
}

buffer *deallocate_buffer(buffer *head, const unsigned i)
{
	buffer *prev = NULL;
	buffer *to_free = head;

	for(unsigned j = 0; j < i; j++) {
		prev = to_free;
		to_free = to_free->next;
	}

	if(prev != NULL) {
		WRMEM(prev->next) = to_free->next;
		rs_free(to_free);
		return head;
	}

	prev = head->next;
	rs_free(head);
	return prev;
}

// CRC table, in static storage, not model-allocated, no __write_mem needed.
static uint32_t crc_table[256];

void crc_table_init(void)
{
	uint32_t n = 256;
	while(n--) {
		uint32_t c = n;
		int k = 8;
		while(k--) {
			if(c & 1)
				c = 0xedb88320UL ^ (c >> 1);
			else
				c = c >> 1;
		}
		crc_table[n] = c;
	}
}

uint32_t crc_update(const uint64_t *buf, size_t n, const uint32_t crc)
{
	uint32_t c = crc ^ 0xffffffffUL;
	while(n--) {
		unsigned k = 64;
		do {
			k -= 8;
			c = crc_table[(c ^ (buf[n] >> k)) & 0xff] ^ (c >> 8);
		} while(k);
	}
	return c ^ 0xffffffffUL;
}

/* -----------------------------------------------------------------------
 * Instrumented ProcessEvent. Replaces application.c for this test.
 * Maybe here a refactor will be useful.
 * -------------------------------------------------------------------- */

#define do_random() (__write_mem(&state->rng_state, sizeof(state->rng_state)), rng_random(&state->rng_state))

void ProcessEvent(const lp_id_t me, const simtime_t now, const unsigned event_type, const void *event_content,
    const unsigned event_size, void *const st)
{
	lp_state *state = st;
	if(state && state->events >= COMPLETE_EVENTS) {
		if(event_type == LP_FINI) {
			if(model_expected_output[me] != state->total_checksum) {
				puts("[ERROR] Incorrect output!");
				abort();
			}
			while(state->head)
				state->head = deallocate_buffer(state->head, 0);
			rs_free(state);
		}
		return;
	}

	if(!state && event_type != LP_INIT) {
		puts("[ERROR] Requested to process a weird event!");
		abort();
	}

	switch(event_type) {
		case LP_INIT:
			state = rs_malloc(sizeof(lp_state));
			if(state == NULL)
				exit(-1);

			/*
			 * memset writes the entire lp_state — notify __write_mem first.
			 * After this, rng_init writes only rng_state (also inside state).
			 */
			WRMEM_BUF(state, sizeof(lp_state));
			memset(state, 0, sizeof(lp_state));

			WRMEM(state->rng_state);
			rng_init(&state->rng_state, ((test_rng_state)me + 1) * 4390023366657240769ULL);
			SetState(state);

			{
				const unsigned buffers_to_allocate = do_random() * MAX_BUFFERS;
				for(unsigned i = 0; i < buffers_to_allocate; ++i) {
					const unsigned c = do_random() * MAX_BUFFER_SIZE / sizeof(uint64_t);
					WRMEM(state->head) = allocate_buffer(state, NULL, c);
					WRMEM(state->buffer_count)++;
				}
			}

			ScheduleNewEvent(me, 20 * do_random(), LOOP, NULL, 0);
			break;

		case LOOP:
			if(do_random() < NULLING_PROBABILITY)
				return;
			WRMEM(state->events)++;
			ScheduleNewEvent(me, now + do_random() * 10, LOOP, NULL, 0);
			{
				lp_id_t dest = do_random() * N_LPS;
				if(do_random() < DOUBLING_PROBABILITY && dest != me)
					ScheduleNewEvent(dest, now + do_random() * 10, LOOP, NULL, 0);

				if(state->buffer_count)
					WRMEM(state->total_checksum) = read_buffer(state->head,
					    do_random() * state->buffer_count, state->total_checksum);

				if(state->buffer_count < MAX_BUFFERS && do_random() < ALLOC_PROBABILITY) {
					const unsigned c = do_random() * MAX_BUFFER_SIZE / sizeof(uint64_t);
					WRMEM(state->head) = allocate_buffer(state, NULL, c);
					WRMEM(state->buffer_count)++;
				}

				if(state->buffer_count && do_random() < DEALLOC_PROBABILITY) {
					WRMEM(state->head) =
					    deallocate_buffer(state->head, do_random() * state->buffer_count);
					WRMEM(state->buffer_count)--;
				}

				if(state->buffer_count && do_random() < SEND_PROBABILITY) {
					const unsigned i = do_random() * state->buffer_count;
					const buffer *to_send = get_buffer(state->head, i);

					dest = do_random() * N_LPS;
					ScheduleNewEvent(dest, now + do_random() * 10, RECEIVE, to_send->data,
					    to_send->count * sizeof(uint64_t));

					WRMEM(state->head) = deallocate_buffer(state->head, i);
					WRMEM(state->buffer_count)--;
				}
			}
			break;

		case RECEIVE:
			if(do_random() < NULLING_PROBABILITY)
				return;
			if(state->buffer_count >= MAX_BUFFERS)
				break;
			WRMEM(state->head) = allocate_buffer(state, event_content, event_size / sizeof(uint64_t));
			WRMEM(state->buffer_count)++;
			break;

		default:
			puts("[ERROR] Requested to process an unknown event!");
			abort();
	}
}

bool CanEnd(lp_id_t me, const void *snapshot)
{
	(void)me;
	const lp_state *state = snapshot;
	return state->events >= COMPLETE_EVENTS;
}
