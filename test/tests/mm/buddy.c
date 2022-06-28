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
#include <stdlib.h>
#include "test.h"

#include "ROOT-Sim.h"
#include "lp/lp.h"
#include "mm/model_allocator.h"
#include "mm/buddy/buddy.h"

#define MAX_ALLOCATION 512

static test_ret_t block_size_test(unsigned b_exp)
{
	test_ret_t errs = 0;
	unsigned block_size = 1 << b_exp;
	unsigned allocations_cnt = 1 << (B_TOTAL_EXP - b_exp);

	uint64_t **allocations = malloc(allocations_cnt * sizeof(uint64_t *));

	for (unsigned i = 0; i < allocations_cnt; ++i) {
		allocations[i] = rs_malloc(block_size);

		if (allocations[i] == NULL) {
			++errs;
			continue;
		}

		for (unsigned j = 0; j < block_size / sizeof(uint64_t); ++j) {
			allocations[i][j] = (uint64_t)(MAX_ALLOCATION * Random());
			//__write_mem(&allocations[i][j], sizeof(allocations[i][j]));
		}
	}

	errs += rs_malloc(block_size) != NULL;

	model_allocator_checkpoint_next_force_full();
	model_allocator_checkpoint_take(0);

	for (unsigned i = 0; i < allocations_cnt; ++i) {
		for (unsigned j = 0; j < block_size / sizeof(uint64_t); ++j) {
			allocations[i][j] = (int)(MAX_ALLOCATION * Random());
			//__write_mem(&allocations[i][j], sizeof(allocations[i][j]));
		}
	}

	errs += rs_malloc(block_size) != NULL;

	model_allocator_checkpoint_take(1);


	for (unsigned i = 0; i < allocations_cnt; ++i) {
		for (unsigned j = 0; j < block_size / sizeof(uint64_t); ++j) {
			allocations[i][j] = (uint64_t)(MAX_ALLOCATION * Random());
//			__write_mem(&allocations[i][j], sizeof(allocations[i][j]));
		}
	}

	errs += rs_malloc(block_size) != NULL;

	model_allocator_checkpoint_take(2);

	model_allocator_checkpoint_restore(1);

	for (unsigned i = 0; i < allocations_cnt; ++i) {
		for (unsigned j = 0; j < block_size / sizeof(uint64_t); ++j) {
			errs += allocations[i][j] != (uint64_t)(MAX_ALLOCATION * Random());
		}

		rs_free(allocations[i]);
	}

	model_allocator_checkpoint_restore(0);

	for (unsigned i = 0; i < allocations_cnt; ++i) {
		for (unsigned j = 0; j < block_size / sizeof(uint64_t); ++j) {
			errs += allocations[i][j] != (uint64_t)(MAX_ALLOCATION * Random());
		}

		rs_free(allocations[i]);
	}

	free(allocations);
	return errs;
}

test_ret_t model_allocator_test(__unused void *_)
{
	test_ret_t errs = 0;

	current_lp = mock_lp();
	model_allocator_lp_init();

	for (unsigned j = B_BLOCK_EXP; j < B_TOTAL_EXP; ++j) {
		errs += block_size_test(j) < 0;
	}

	errs += rs_malloc(0) != NULL;
	errs += rs_calloc(0, sizeof(uint64_t)) != NULL;

	rs_free(NULL);

	int64_t *mem = rs_calloc(1, sizeof(uint64_t));
	errs += *(test_ret_t *)mem;
	rs_free(mem);

	model_allocator_lp_fini();

	return errs;
}
