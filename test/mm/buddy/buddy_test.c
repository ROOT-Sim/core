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

static __thread test_rng_state rng_state;

__thread struct lp_ctx *current_lp; // needed by the model allocator
__thread simtime_t current_gvt; // needed by the model allocator

static int block_size_test(unsigned b_exp)
{
	unsigned errs = 0;
	unsigned block_size = 1 << b_exp;
	unsigned allocations_cnt = 1 << (B_TOTAL_EXP - b_exp);
	test_rng_state rng_snap_a = rng_state;
	uint64_t **allocations = malloc(allocations_cnt * sizeof(uint64_t *));

	for (unsigned i = 0; i < allocations_cnt; ++i) {
		allocations[i] = malloc_mt(block_size);

		if (allocations[i] == NULL) {
			++errs;
			continue;
		}

		for (unsigned j = 0; j < block_size / sizeof(uint64_t); ++j) {
			allocations[i][j] = lcg_random_u(rng_state);
			__write_mem(&allocations[i][j], sizeof(allocations[i][j]));
		}
	}

	errs += malloc_mt(block_size) != NULL;

	model_allocator_checkpoint_next_force_full();
	model_allocator_checkpoint_take(0);
	test_rng_state rng_snap_b = rng_state;

	for (unsigned i = 0; i < allocations_cnt; ++i) {
		for (unsigned j = 0; j < block_size / sizeof(uint64_t); ++j) {
			allocations[i][j] = lcg_random_u(rng_state);
			__write_mem(&allocations[i][j], sizeof(allocations[i][j]));
		}
	}

	errs += malloc_mt(block_size) != NULL;

	model_allocator_checkpoint_take(B_LOG_FREQUENCY);


	for (unsigned i = 0; i < allocations_cnt; ++i) {
		for (unsigned j = 0; j < block_size / sizeof(uint64_t); ++j) {
			allocations[i][j] = lcg_random_u(rng_state);
			__write_mem(&allocations[i][j], sizeof(allocations[i][j]));
		}
	}

	errs += malloc_mt(block_size) != NULL;

	model_allocator_checkpoint_take(B_LOG_FREQUENCY * 2 - 1);
	model_allocator_checkpoint_take(B_LOG_FREQUENCY * 2);

	model_allocator_checkpoint_restore(B_LOG_FREQUENCY);

	for (unsigned i = 0; i < allocations_cnt; ++i) {
		for (unsigned j = 0; j < block_size / sizeof(uint64_t); ++j) {
			errs += allocations[i][j] != lcg_random_u(rng_snap_b);
		}

		free_mt(allocations[i]);
	}

	model_allocator_checkpoint_restore(0);

	for (unsigned i = 0; i < allocations_cnt; ++i) {
		for (unsigned j = 0; j < block_size / sizeof(uint64_t); ++j) {
			errs += allocations[i][j] != lcg_random_u(rng_snap_a);
		}

		free_mt(allocations[i]);
	}

	free(allocations);
	return -(!!errs);
}

static int model_allocator_test(void)
{
	current_lp = malloc(sizeof(*current_lp));
	model_allocator_lp_init();
	lcg_init(rng_state, (rid + 1) * 1713);

	unsigned errs = 0;
	for (unsigned j = B_BLOCK_EXP; j < B_TOTAL_EXP; ++j) {
		errs += block_size_test(j) < 0;
	}

	errs += malloc_mt(0) != NULL;
	errs += calloc_mt(0, sizeof(uint64_t)) != NULL;

	free_mt(NULL);

	uint64_t *mem = calloc_mt(1, sizeof(uint64_t));
	errs += *mem;
	free_mt(mem);

	model_allocator_lp_fini();
	free(current_lp);
	return -(!!errs);
}

const struct test_config test_config = {
	.threads_count = 4,
	.test_fnc = model_allocator_test
};

