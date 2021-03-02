/**
 * @file test/integration/model/application.h
 *
 * @brief Header of the model used to verify the runtime correctness
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <ROOT-Sim.h>

#include <test_rng.h>

#define MAX_BUFFERS 256
#define MAX_BUFFER_SIZE 512
#define SEND_PROBABILITY 0.05
#define ALLOC_PROBABILITY 0.2
#define DEALLOC_PROBABILITY 0.2
#define DOUBLING_PROBABILITY 0.5
#define COMPLETE_EVENTS 10000

enum {
	LOOP = MODEL_FINI + 1,
	RECEIVE
};

typedef struct _buffer {
	unsigned count;
	struct _buffer *next;
	uint64_t data[];
} buffer;

typedef struct _lp_state_type {
	unsigned events;
	unsigned buffer_count;
	uint32_t total_checksum;
	test_rng_state rng_state;
	buffer *head;
} lp_state;

buffer* get_buffer(buffer *head, unsigned i);
uint32_t read_buffer(buffer *head, unsigned i, uint32_t old_crc);
buffer* allocate_buffer(lp_state *state, const unsigned *data, unsigned count);
buffer* deallocate_buffer(buffer *head, unsigned i);
void crc_table_init(void);

extern const char model_expected_output_64[679];
