/**
 * @file test/mm/buddy/buddy_test.c
 *
 * @brief Test: rollbackable buddy system allocator
 *
 * A test of the buddy system allocator used to handle model's memory
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <test.h>
#include <test_rng.h>

#include <lp/lp.h>
#include <mm/model_allocator.h>

#include <stdlib.h>

#ifndef ROOTSIM_INCREMENTAL
#define __write_mem(x, y)
#endif

#define MAX_ALLOC_CNT 100
#define MAX_ALLOC_S 1000

#define MAX_ALLOC_PHASES 1000
#define MAX_ALLOC_STEP 50
#define ALLOC_OSCILLATIONS 100

#define FULL_CHK_P 0.2

#define MAX_ALLOC_E (MAX_ALLOC_S / sizeof(unsigned))

static __thread test_rng_state rng_state;

__thread struct lp_ctx *current_lp; // needed by the model allocator
__thread simtime_t current_gvt; // needed by the model allocator

/// A single tested allocation
struct alc {
	/// The pointer to the memory returned by the tested memory allocator
	unsigned *ptr;
	/// The size expressed in number of entries of @a data and @a ptr
	unsigned c;
	/// The data we expect to find in this allocation
	unsigned data[MAX_ALLOC_E];
};

static void allocation_init(struct alc *alc)
{
	unsigned c = ((lcg_random_u(rng_state) % MAX_ALLOC_E) + 1);

	alc->ptr = malloc_mt(c * sizeof(*alc->ptr));
	if(alc->ptr == NULL) {
		abort();
	}
	alc->c = c;
	__write_mem(alc->ptr, alc->c * sizeof(unsigned));

	while(c--) {
		unsigned v = lcg_random_u(rng_state);
		alc->ptr[c] = v;
		alc->data[c] = v;
	}
}

static struct alc *allocation_all_init(void)
{
	struct alc *ret = malloc(sizeof(*ret) * MAX_ALLOC_CNT * MAX_ALLOC_PHASES);
	memset(ret, 0, sizeof(*ret));

	unsigned i = MAX_ALLOC_CNT;
	while(i--) {
		allocation_init(&ret[i]);
	}

	return ret;
}

static void allocation_all_fini(struct alc *alc)
{
	unsigned i = MAX_ALLOC_CNT;
	while(i--) {
		free_mt(alc[i].ptr);
	}
	free(alc);
}

static void allocation_partial_write(struct alc *alc, unsigned p)
{
	alc += p * MAX_ALLOC_CNT;

	memcpy(alc, alc - MAX_ALLOC_CNT, MAX_ALLOC_CNT * sizeof(*alc));

	unsigned w = lcg_random_u(rng_state) % (MAX_ALLOC_CNT / 2);
	while(w--) {
		unsigned i = lcg_random_u(rng_state) % MAX_ALLOC_CNT;

		if(alc[i].ptr == NULL) {
			continue;
		}

		unsigned c = alc[i].c;
		unsigned e = lcg_random_u(rng_state) % (c + 1);
		unsigned l = lcg_random_u(rng_state) % (e + 1);

		__write_mem(alc[i].ptr + l, (e - l) * sizeof(unsigned));

		for(unsigned j = l; j < e; ++j) {
			unsigned v = lcg_random_u(rng_state);
			alc[i].ptr[j] = v;
			alc[i].data[j] = v;
		}
	}

	w = lcg_random_u(rng_state) % (MAX_ALLOC_CNT / 6);
	while(w--) {
		unsigned i = lcg_random_u(rng_state) % MAX_ALLOC_CNT;
		if(alc[i].ptr == NULL) {
			allocation_init(&alc[i]);
		} else {
			free_mt(alc[i].ptr);
			alc[i].ptr = NULL;
			alc[i].c = 0;
		}
	}
}

static bool allocation_check(struct alc *alc, unsigned p)
{
	alc += p * MAX_ALLOC_CNT;

	unsigned i = MAX_ALLOC_CNT;
	while(i--) {
		if(alc[i].ptr == NULL) {
			if(alc[i].c != 0) {
				return true;
			}
		} else {
			if(memcmp(alc[i].ptr, alc[i].data, alc[i].c * sizeof(unsigned))) {
				return true;
			}
		}
	}
	return false;
}

static bool allocation_cycle(struct alc *alc, unsigned c, unsigned up, unsigned down)
{
	for(unsigned i = c + 1; i <= up; ++i) {
		allocation_partial_write(alc, i);
		if(allocation_check(alc, i)) {
			return true;
		}
		if(lcg_random(rng_state) < FULL_CHK_P) {
			model_allocator_checkpoint_next_force_full();
		}
		model_allocator_checkpoint_take(i);
	}

	up = max(up, c);

	while(1) {
		model_allocator_checkpoint_restore(up);

		if(allocation_check(alc, up)) {
			return true;
		}

		unsigned s = (lcg_random_u(rng_state) % MAX_ALLOC_STEP) + 1;

		if(up <= down + s) {
			break;
		}
		up -= s;
	}

	model_allocator_checkpoint_restore(down);
	return allocation_check(alc, down);
}

static int model_allocator_test(void)
{
	current_lp = malloc(sizeof(*current_lp));
	model_allocator_init();
	model_allocator_lp_init();
	lcg_init(rng_state, (rid + 1) * 1713);

	struct alc *alc = allocation_all_init();

	model_allocator_checkpoint_next_force_full();
	model_allocator_checkpoint_take(0);

	if(allocation_check(alc, 0)) {
		return -1;
	}

	unsigned c = 0;
	for(unsigned j = 0; j < ALLOC_OSCILLATIONS; ++j) {

		unsigned u = (lcg_random_u(rng_state) % (MAX_ALLOC_PHASES - 1)) + 1;
		unsigned d = (lcg_random_u(rng_state) % u);

		if(allocation_cycle(alc, c, u, d)) {
			return -1;
		}

		c = d;
	}

	model_allocator_checkpoint_restore(0);

	allocation_all_fini(alc);

	model_allocator_lp_fini();
	model_allocator_fini();
	free(current_lp);

	return 0;
}
