/**
* @file test/tests/mm/stress.c
*
* @brief Test: stress test the model memory allocator
*
* Multiple stress tests of the model memory allocator. Based on t-test1 by Wolfram Glager
*
* SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
* SPDX-License-Identifier: GPL-3.0-only
*/

#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <pthread.h>
#include <stdlib.h>

#include "mm/mm.h"
#include <string.h>
#include "test.h"
#include "lp/lp.h"

#define ACTIONS_MAX	100
#define I_MAX		10000


/* For large allocation sizes, the time required by copying in
   realloc() can dwarf all other execution times.  Avoid this with a
   size threshold. */
#define REALLOC_MAX	2000

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
	size_t i, j;

	memset(ptr, 0, size);

	if (!size)
		return;
	for (i = 0; i < size; i += 2047) {
		j = (size_t)ptr ^ i;
		ptr[i] = j ^ (j >> 8);
	}
	j = (size_t)ptr ^ (size - 1);
	ptr[size - 1] = j ^ (j >> 8);
}

static int mem_check(const unsigned char *ptr, size_t size)
{
	size_t i, j;

	if (!size)
		return 0;
	for (i = 0; i < size; i += 2047) {
		j = (size_t)ptr ^ i;
		if (ptr[i] != ((j ^ (j >> 8)) & 0xFF))
			return 1;
	}
	j = (size_t)ptr ^ (size - 1);
	if (ptr[size - 1] != ((j ^ (j >> 8)) & 0xFF))
		return 2;
	return 0;
}

static int zero_check(void *p, size_t size)
{
	unsigned *ptr = p;
	unsigned char *ptr2;

	while (size >= sizeof(*ptr)) {
		if (*ptr++)
			return -1;
		size -= sizeof(*ptr);
	}
	ptr2 = (unsigned char *)ptr;

	while (size > 0) {
		if (*ptr2++)
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

	if (r < 120) { // calloc
		if (m->size > 0)
			rs_free(m->ptr);
		m->ptr = rs_calloc(size, 1);
		test_assert(m->ptr);
		test_assert(zero_check(m->ptr, size) == 0);

	} else if (r < 200 && m->size > 0 && m->size < REALLOC_MAX) { // realloc
		m->ptr = rs_realloc(m->ptr, size);
		test_assert(m->ptr);
	} else {
		// malloc
		if (m->size > 0)
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
	if (!m->size)
		return;

	test_assert(mem_check(m->ptr, m->size) == 0);

	rs_free(m->ptr);
	m->size = 0;
}

static void bin_test(struct bin_info *p)
{
	size_t b;

	for (b = 0; b < p->bins; b++) {
		test_assert(mem_check(p->m[b].ptr, p->m[b].size) == 0);
	}
}

test_ret_t parallel_malloc_test(__unused void *_)
{
	unsigned i, b, j, actions, action;

	// Mock a fake LP (per-thread)
	struct lp_ctx lp = {0};
	struct lib_ctx ctx = {0};
	lp.lib_ctx = &ctx;
	lp.lib_ctx->rng_s[0] = 7319936632422683443ULL;
	lp.lib_ctx->rng_s[1] = 2268344373199366324ULL;
	lp.lib_ctx->rng_s[2] = 3443862242366399137ULL;
	lp.lib_ctx->rng_s[3] = 2366399137344386224ULL;
	current_lp = &lp;
	model_allocator_lp_init();

	struct bin_info p;
	p.size = (1 << (B_BLOCK_EXP + 1));
	p.bins = (1 << B_TOTAL_EXP) / p.size;
	p.m = malloc(p.bins * sizeof(*p.m));

	for (b = 0; b < p.bins; b++) {
		p.m[b].size = 0;
		p.m[b].ptr = NULL;
		if (!RANDOM(2))
			bin_alloc(&p.m[b], RANDOM(p.size) + 1, test_random());
	}

	for (i = 0; i <= I_MAX; ) {
		bin_test(&p);

		actions = RANDOM(ACTIONS_MAX);

		for (j = 0; j < actions; j++) {
			b = RANDOM(p.bins);
			bin_free(&p.m[b]);
		}
		i += actions;
		actions = RANDOM(ACTIONS_MAX);

		for (j = 0; j < actions; j++) {
			b = RANDOM(p.bins);
			action = test_random();
			bin_alloc(&p.m[b], RANDOM(p.size) + 1, action);
			bin_test(&p);
		}

		i += actions;
	}

	for (b = 0; b < p.bins; b++)
		bin_free(&p.m[b]);

	free(p.m);

	model_allocator_lp_fini();

	return 0;
}
