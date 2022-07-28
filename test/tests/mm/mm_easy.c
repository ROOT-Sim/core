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

static int block_size_test(unsigned b_exp)
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

	model_allocator_checkpoint_next_force_full();
	model_allocator_checkpoint_take(0);
	b_chk = b_rng;

	for(unsigned i = 0; i < allocations_cnt; ++i) {
		for(unsigned j = 0; j < block_size / sizeof(uint64_t); ++j) {
			allocations[i][j] = rng_random_u(&b_rng);
			//__write_mem(&allocations[i][j], sizeof(allocations[i][j]));
		}
	}

	model_allocator_checkpoint_take(1);

	for(unsigned i = 0; i < allocations_cnt; ++i) {
		for(unsigned j = 0; j < block_size / sizeof(uint64_t); ++j) {
			allocations[i][j] = rng_random_u(&b_rng);
			//__write_mem(&allocations[i][j], sizeof(allocations[i][j]));
		}
	}

	model_allocator_checkpoint_take(2);

	model_allocator_checkpoint_restore(1);
	b_rng = b_chk;

	for(unsigned i = 0; i < allocations_cnt; ++i) {
		for(unsigned j = 0; j < block_size / sizeof(uint64_t); ++j) {
			errs += allocations[i][j] != rng_random_u(&b_rng);
		}

		rs_free(allocations[i]);
	}

	model_allocator_checkpoint_restore(0);
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

int model_allocator_test(_unused void *_)
{
	int errs = 0;

	current_lp = test_lp_mock_get();
	model_allocator_lp_init();

	for(unsigned j = 6; j < 16; ++j) {
		errs += block_size_test(j);
	}

	errs += rs_malloc(0) != NULL;
	errs += rs_calloc(0, sizeof(uint64_t)) != NULL;

	rs_free(NULL);

	int64_t *mem = rs_calloc(1, sizeof(uint64_t));
	errs += *mem != 0;
	rs_free(mem);

	model_allocator_lp_fini();

	return errs;
}
