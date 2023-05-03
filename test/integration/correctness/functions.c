/**
 * @file test/tests/integration/correctness/functions.c
 *
 * @brief Helper functions of the model used to verify the runtime correctness
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include "application.h"

#include <string.h>

uint32_t crc_update(const uint64_t *buf, size_t n, uint32_t crc);

buffer *get_buffer(buffer *head, unsigned i)
{
	while(i--)
		head = head->next;

	return head;
}

uint32_t read_buffer(buffer *head, unsigned i, uint32_t old_crc)
{
	head = get_buffer(head, i);
	return crc_update(head->data, head->count, old_crc);
}

buffer *allocate_buffer(lp_state *state, const unsigned *data, unsigned count)
{
	buffer *new = rs_malloc(sizeof(buffer) + count * sizeof(uint64_t));
	new->next = state->head;
	new->count = count;

	if(data != NULL)
		memcpy(new->data, data, count * sizeof(uint64_t));
	else
		for(unsigned i = 0; i < count; i++)
			new->data[i] = rng_random_u(&state->rng_state);

	return new;
}

buffer *deallocate_buffer(buffer *head, unsigned i)
{
	buffer *prev = NULL;
	buffer *to_free = head;

	for(unsigned j = 0; j < i; j++) {
		prev = to_free;
		to_free = to_free->next;
	}

	if(prev != NULL) {
		prev->next = to_free->next;
		rs_free(to_free);
		return head;
	}

	prev = head->next;
	rs_free(head);
	return prev;
}

static uint32_t crc_table[256];

void crc_table_init(void)
{
	uint32_t n = 256;
	while(n--) {
		uint32_t c = n;
		int k = 8;
		while(k--) {
			if(c & 1) {
				c = 0xedb88320UL ^ (c >> 1);
			} else {
				c = c >> 1;
			}
		}
		crc_table[n] = c;
	}
}

uint32_t crc_update(const uint64_t *buf, size_t n, uint32_t crc)
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
