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
#define NULLING_PROBABILITY 0.3
#define COMPLETE_EVENTS 15000

enum {
	LOOP,
	RECEIVE
};

typedef struct lp_buffer {
	unsigned count;
	struct lp_buffer *next;
	uint64_t data[];
} buffer;

typedef struct {
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

extern const uint32_t model_expected_output[];
