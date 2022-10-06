/**
 * @file test/tests/mm/buddy.c
 *
 * @brief Test: rollbackable buddy system allocator
 *
 * A test of the buddy system allocator used to handle model's memory
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <test.h>

#include <framework/rng.h>

#include <lp/lp.h>
#include <mm/model_allocator.h>

#include <stdlib.h>

#define BUDDY_TEST_SEED 0x5E550UL

static int block_size_test(struct mm_state *mm_state, unsigned b_exp)
{
	int errs = 0;
	unsigned block_size = 1 << b_exp;
	unsigned allocations_cnt = 1 << (24 - b_exp);
	test_rng_state b_rng, b_chk;
	rng_init(&b_rng, BUDDY_TEST_SEED);
	uint64_t **allocations = malloc(allocations_cnt * sizeof(uint64_t *));

	for(unsigned i = 0; i < allocations_cnt; ++i) {
		allocations[i] = rs_malloc(block_size);

		if(allocations[i] == NULL) {
			++errs;
			continue;
		}

		for(unsigned j = 0; j < block_size / sizeof(uint64_t); ++j) {
			allocations[i][j] = rng_random_u(&b_rng);
			//__write_mem(&allocations[i][j], sizeof(allocations[i][j]));
		}
	}

	model_allocator_checkpoint_next_force_full(mm_state);
	model_allocator_checkpoint_take(mm_state, 0);
	b_chk = b_rng;

	for(unsigned i = 0; i < allocations_cnt; ++i) {
		for(unsigned j = 0; j < block_size / sizeof(uint64_t); ++j) {
			allocations[i][j] = rng_random_u(&b_rng);
			//__write_mem(&allocations[i][j], sizeof(allocations[i][j]));
		}
	}

	model_allocator_checkpoint_take(mm_state, 1);

	for(unsigned i = 0; i < allocations_cnt; ++i) {
		for(unsigned j = 0; j < block_size / sizeof(uint64_t); ++j) {
			allocations[i][j] = rng_random_u(&b_rng);
			//__write_mem(&allocations[i][j], sizeof(allocations[i][j]));
		}
	}

	model_allocator_checkpoint_take(mm_state, 2);

	model_allocator_checkpoint_restore(mm_state, 1);
	b_rng = b_chk;

	for(unsigned i = 0; i < allocations_cnt; ++i) {
		for(unsigned j = 0; j < block_size / sizeof(uint64_t); ++j) {
			errs += allocations[i][j] != rng_random_u(&b_rng);
		}

		rs_free(allocations[i]);
	}

	model_allocator_checkpoint_restore(mm_state, 0);
	rng_init(&b_rng, BUDDY_TEST_SEED);

	for(unsigned i = 0; i < allocations_cnt; ++i) {
		for(unsigned j = 0; j < block_size / sizeof(uint64_t); ++j) {
			errs += allocations[i][j] != rng_random_u(&b_rng);
		}

		rs_free(allocations[i]);
	}

	free(allocations);
	return errs > 0;
}

static int blocks_full_test(void)
{
	int errs = 0;
	for(unsigned j = 6; j < 16; ++j) {
		errs += block_size_test(&current_lp->mm_state, j);
	}

	errs += rs_malloc(0) != NULL;
	errs += rs_calloc(0, sizeof(uint64_t)) != NULL;

	rs_free(NULL);

	int64_t *mem = rs_calloc(1, sizeof(uint64_t));
	errs += *mem != 0;
	rs_free(mem);
	return errs;
}

int model_allocator_test(_unused void *_)
{
	int errs = 0;

	current_lp = test_lp_mock_get();
	for(enum mm_allocator_choice mm = MM_MULTI_BUDDY; mm <= MM_DYMELOR; ++mm) {
		global_config.mm = MM_MULTI_BUDDY;
		model_allocator_lp_init(&current_lp->mm_state);
		errs += blocks_full_test();
		model_allocator_lp_fini(&current_lp->mm_state);
	}
	return errs;
}
