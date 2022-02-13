/**
 * @file test/mm/buddy.c
 *
 * @brief Test: rollbackable buddy system allocator
 *
 * A test of the buddy system allocator used to handle model's memory
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <stdlib.h>
#include <test.h>

#include <ROOT-Sim.h>
#include <lp/lp.h>
#include <mm/model_allocator.h>
#include <mm/buddy/buddy.h>

#define MAX_ALLOCATION 1024

static int block_size_test(unsigned b_exp)
{
	unsigned errs = 0;
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
			allocations[i][j] = (int)(MAX_ALLOCATION * Random());
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
			allocations[i][j] = (int)(MAX_ALLOCATION * Random());
			//__write_mem(&allocations[i][j], sizeof(allocations[i][j]));
		}
	}

	errs += rs_malloc(block_size) != NULL;

	model_allocator_checkpoint_take(2);

	model_allocator_checkpoint_restore(1);

	for (unsigned i = 0; i < allocations_cnt; ++i) {
		for (unsigned j = 0; j < block_size / sizeof(uint64_t); ++j) {
			errs += allocations[i][j] != (int)(MAX_ALLOCATION * Random());
		}

		rs_free(allocations[i]);
	}

	model_allocator_checkpoint_restore(0);

	for (unsigned i = 0; i < allocations_cnt; ++i) {
		for (unsigned j = 0; j < block_size / sizeof(uint64_t); ++j) {
			errs += allocations[i][j] != (int)(MAX_ALLOCATION * Random());
		}

		rs_free(allocations[i]);
	}

	free(allocations);
	return -(errs != 0);
}

static test_ret_t model_allocator_test(__unused void *_)
{
	current_lp = malloc(sizeof(*current_lp));
	model_allocator_lp_init();

	unsigned errs = 0;
	for (unsigned j = B_BLOCK_EXP; j < B_TOTAL_EXP; ++j) {
		errs += block_size_test(j) < 0;
	}

	errs += rs_malloc(0) != NULL;
	errs += rs_calloc(0, sizeof(uint64_t)) != NULL;

	rs_free(NULL);

	uint64_t *mem = rs_calloc(1, sizeof(uint64_t));
	errs += *mem;
	rs_free(mem);

	return -(errs != 0);
}



int main(void)
{
	init(0);
	srand(time(NULL));

	// Mock a fake LP
	struct lp_ctx lp = {0};
	lp.lib_ctx = malloc(sizeof(*current_lp->lib_ctx));
	lp.lib_ctx->rng_s[0] = 7319936632422683443ULL;
	lp.lib_ctx->rng_s[1] = 2268344373199366324ULL;
	lp.lib_ctx->rng_s[2] = 3443862242366399137ULL;
	lp.lib_ctx->rng_s[3] = 2366399137344386224ULL;
	current_lp = &lp;

	test("Testing buddy system", model_allocator_test, NULL);

	free(lp.lib_ctx);
	finish();
}
