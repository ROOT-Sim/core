/**
 * @file test/mm/parallel.c
 *
 * @brief Test: stress test the model memory allocator
 *
 * Multiple stress tests of the model memory allocator. Based on t-test1 by Wolfram Glager
 *
 * SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <test.h>

#include <lp/lp.h>
#include <mm/buddy/buddy.h>
#include "mock.h"

#include <string.h>

#define ACTIONS_MAX 100
#define I_MAX 10000


/* For large allocation sizes, the time required by copying in
   realloc() can dwarf all other execution times.  Avoid this with a
   size threshold. */
#define REALLOC_MAX 2000

struct bin {
	unsigned char *ptr;
	size_t size;
};

struct bin_info {
	struct bin *m;
	size_t size, bins;
};

static void mem_init(unsigned char *ptr, size_t size)
{
	if(!size)
		return;

	memset(ptr, 0, size);

	size_t j;
	for(size_t i = 0; i < size; i += 2047) {
		j = (size_t)ptr ^ i;
		ptr[i] = j ^ (j >> 8);
	}
	j = (size_t)ptr ^ (size - 1);
	ptr[size - 1] = j ^ (j >> 8);
}

static int mem_check(const unsigned char *ptr, size_t size)
{
	if(!size)
		return 0;

	size_t j;
	for(size_t i = 0; i < size; i += 2047) {
		j = (size_t)ptr ^ i;
		if(ptr[i] != ((j ^ (j >> 8)) & 0xFF))
			return 1;
	}
	j = (size_t)ptr ^ (size - 1);
	if(ptr[size - 1] != ((j ^ (j >> 8)) & 0xFF))
		return 2;
	return 0;
}

static int zero_check(void *p, size_t size)
{
	unsigned *ptr = p;
	while(size >= sizeof(*ptr)) {
		if(*ptr++)
			return -1;
		size -= sizeof(*ptr);
	}

	unsigned char *ptr2 = (unsigned char *)ptr;
	while(size > 0) {
		if(*ptr2++)
			return -1;
		--size;
	}
	return 0;
}

/*
 * Allocate a bin with malloc(), realloc() or memalign().
 * r must be a random number >= 1024.
 */
static void bin_alloc(struct bin *m, size_t size, unsigned r)
{
	test_assert(mem_check(m->ptr, m->size) == 0);

	r %= 1024;

	if(r < 120) { // calloc
		if(m->size > 0)
			rs_free(m->ptr);
		m->ptr = rs_calloc(size, 1);
		test_assert(m->ptr);
		test_assert(zero_check(m->ptr, size) == 0);

	} else if(r < 200 && m->size > 0 && m->size < REALLOC_MAX) { // realloc
		m->ptr = rs_realloc(m->ptr, size);
		test_assert(m->ptr);
	} else { // malloc
		if(m->size > 0)
			rs_free(m->ptr);
		m->ptr = rs_malloc(size);
		test_assert(m->ptr);
	}

	m->size = size;
	mem_init(m->ptr, m->size);
}

/* Free a bin. */
static void bin_free(struct bin *m)
{
	if(!m->size)
		return;

	test_assert(mem_check(m->ptr, m->size) == 0);

	rs_free(m->ptr);
	m->size = 0;
}

static void bin_test(struct bin_info *p)
{
	for(size_t b = 0; b < p->bins; b++)
		test_assert(mem_check(p->m[b].ptr, p->m[b].size) == 0);
}

int parallel_malloc_test(_unused void *_)
{
	struct lp_ctx *lp = test_lp_mock_get();
	current_lp = lp;
	model_allocator_lp_init(&lp->mm_state);

	struct bin_info p;
	p.size = (1 << (B_BLOCK_EXP + 1));
	p.bins = (1 << B_TOTAL_EXP) / p.size;
	p.m = malloc(p.bins * sizeof(*p.m));

	for(unsigned b = 0; b < p.bins; b++) {
		p.m[b].size = 0;
		p.m[b].ptr = NULL;
		if(!test_random_range(2))
			bin_alloc(&p.m[b], test_random_range(p.size) + 1, test_random_u());
	}

	for(unsigned i = 0; i <= I_MAX;) {
		bin_test(&p);

		unsigned actions = test_random_range(ACTIONS_MAX);

		for(unsigned j = 0; j < actions; j++) {
			unsigned bin = test_random_range(p.bins);
			bin_free(&p.m[bin]);
		}
		i += actions;
		actions = test_random_range(ACTIONS_MAX);

		for(unsigned j = 0; j < actions; j++) {
			unsigned bin = test_random_range(p.bins);
			uint64_t action = test_random_u();
			bin_alloc(&p.m[bin], test_random_range(p.size) + 1, action);
			bin_test(&p);
		}

		i += actions;
	}

	for(unsigned b = 0; b < p.bins; b++)
		bin_free(&p.m[b]);

	free(p.m);

	model_allocator_lp_fini(&lp->mm_state);

	return 0;
}
