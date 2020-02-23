#include <test.h>
#include <test_rng.h>
#include <mm/model_allocator.h>
#include <lp/lp.h>

#include <stdlib.h>
#include <stdint.h>

static __thread uint128_t rng_state;

static int block_size_test(unsigned b_exp)
{
	int ret = -1;
	unsigned block_size = 1 << b_exp;
	unsigned allocations_cnt = 1 << (B_TOTAL_EXP - b_exp);
	uint128_t rng_state_snap = rng_state;
	mm_checkpoint *ckp1 = NULL;
	mm_checkpoint *ckp2 = NULL;
	uint64_t **allocations = malloc(allocations_cnt * sizeof(uint64_t *));

	for (unsigned i = 0; i < allocations_cnt; ++i) {
		allocations[i] = model_alloc(block_size);

		if (allocations[i] == NULL)
			goto error;

		for (unsigned j = 0; j < block_size/sizeof(uint64_t); ++j) {
			allocations[i][j] = lcg_random_u(rng_state);
		}
	}

	ckp1 = model_checkpoint_take();

	for (unsigned i = 0; i < allocations_cnt; ++i) {
		for (unsigned j = 0; j < block_size/sizeof(uint64_t); ++j) {
			allocations[i][j] = lcg_random_u(rng_state);
		}
	}

	ckp2 = model_checkpoint_take();
	model_checkpoint_restore(ckp1);

	for (unsigned i = 0; i < allocations_cnt; ++i) {
		for (unsigned j = 0; j < block_size/sizeof(uint64_t); ++j) {
			if (allocations[i][j] != lcg_random_u(rng_state_snap))
				goto error;
		}

		model_free(allocations[i]);
	}

	model_checkpoint_restore(ckp2);

	for (unsigned i = 0; i < allocations_cnt; ++i) {
		for (unsigned j = 0; j < block_size/sizeof(uint64_t); ++j) {
			if (allocations[i][j] != lcg_random_u(rng_state_snap))
				goto error;
		}

		model_free(allocations[i]);
	}

	ret = 0;
	error:
	free(allocations);
	model_checkpoint_free(ckp1);
	model_checkpoint_free(ckp2);
	return ret;
}

static int model_allocator_test(unsigned thread_id)
{
	int ret = -1;

	current_lp = malloc(sizeof(*current_lp));
	model_memory_init();
	lcg_init(rng_state, (thread_id + 1) * 1713);

	for (unsigned j = B_BLOCK_EXP; j < B_TOTAL_EXP; ++j) {
		if (block_size_test(j) < 0)
			goto error;
	}

	ret = 0;
	error:
	model_memory_fini();
	free(current_lp);
	return ret;
}

struct _test_config_t test_config = {
	.test_name = "model allocator",
	.threads_count = 4,
	.test_fnc = model_allocator_test
};

