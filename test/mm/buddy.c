/**
 * @file test/mm/buddy.c
 *
 * @brief Test: rollbackable buddy system allocator
 *
 * A test of the buddy system allocator used to handle model's memory
 *
 * SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <test.h>

#include <lp/lp.h>
#include <mm/buddy/buddy.h>
#include <mock.h>

#include <stdlib.h>

#define BUDDY_TEST_SEED 0x5E550UL

static void write_allocations(uint64_t **allocations, unsigned allocations_cnt, unsigned block_size,
    test_rng_state *b_rng_p)
{
	for(unsigned i = 0; i < allocations_cnt; ++i)
		for(unsigned j = 0; j < block_size / sizeof(uint64_t); ++j)
			allocations[i][j] = rng_random_u(b_rng_p);
}

static int check_and_free_allocations(uint64_t **allocations, unsigned allocations_cnt, unsigned block_size,
    test_rng_state *b_rng_p)
{
	int errs = 0;
	for(unsigned i = 0; i < allocations_cnt; ++i) {
		for(unsigned j = 0; j < block_size / sizeof(uint64_t); ++j)
			errs += allocations[i][j] != rng_random_u(b_rng_p);

		rs_free(allocations[i]);
	}
	return errs;
}

static int block_size_test(struct mm_state *mm, unsigned b_exp)
{
	int errs = 0;
	unsigned block_size = 1 << b_exp;
	unsigned allocations_cnt = 1 << (B_TOTAL_EXP - b_exp);
	test_rng_state b_rng, b_chk;
	rng_init(&b_rng, BUDDY_TEST_SEED);
	uint64_t **allocations = malloc(allocations_cnt * sizeof(uint64_t *));

	for(unsigned i = 0; i < allocations_cnt; ++i) {
		allocations[i] = rs_malloc(block_size);
		errs += allocations[i] == NULL;
	}
	write_allocations(allocations, allocations_cnt, block_size, &b_rng);

	model_allocator_checkpoint_take(mm, 0);
	b_chk = b_rng;

	write_allocations(allocations, allocations_cnt, block_size, &b_rng);
	model_allocator_checkpoint_take(mm, 1);

	write_allocations(allocations, allocations_cnt, block_size, &b_rng);
	model_allocator_checkpoint_take(mm, 2);

	model_allocator_checkpoint_restore(mm, 1);
	b_rng = b_chk;
	errs += check_and_free_allocations(allocations, allocations_cnt, block_size, &b_rng);

	model_allocator_checkpoint_restore(mm, 0);
	rng_init(&b_rng, BUDDY_TEST_SEED);
	errs += check_and_free_allocations(allocations, allocations_cnt, block_size, &b_rng);

	free(allocations);
	return errs > 0;
}

int model_allocator_test(_unused void *_)
{
	int errs = 0;

	struct lp_ctx *lp = test_lp_mock_get();
	current_lp = lp;
	model_allocator_lp_init(&lp->mm_state);

	for(unsigned j = B_BLOCK_EXP; j < B_TOTAL_EXP; ++j)
		errs += block_size_test(&lp->mm_state, j);

	errs += rs_malloc(0) != NULL;
	errs += rs_calloc(0, sizeof(uint64_t)) != NULL;

	rs_free(NULL);

	int64_t *mem = rs_calloc(1, sizeof(uint64_t));
	errs += *mem != 0;
	rs_free(mem);

	model_allocator_lp_fini(&lp->mm_state);

	return errs;
}
