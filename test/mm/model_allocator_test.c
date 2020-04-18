#include <test.h>
#include <test_rng.h>
#include <mm/model_allocator.h>
#include <lp/lp.h>

#include <stdlib.h>
#include <stdint.h>

static __thread uint128_t rng_state;

__thread lp_struct *current_lp; // needed by the model allocator

static int block_size_test(unsigned b_exp)
{
	unsigned errs = 0;
	unsigned block_size = 1 << b_exp;
	unsigned allocations_cnt = 1 << (B_TOTAL_EXP - b_exp);
	uint128_t rng_state_snap = rng_state;
	uint64_t **allocations = malloc(allocations_cnt * sizeof(uint64_t *));

	for (unsigned i = 0; i < allocations_cnt; ++i) {
		allocations[i] = __wrap_malloc(block_size);

		if (allocations[i] == NULL) {
			++errs;
			continue;
		}

		for (unsigned j = 0; j < block_size/sizeof(uint64_t); ++j) {
			allocations[i][j] = lcg_random_u(rng_state);
		}
	}

	errs += __wrap_malloc(block_size) != NULL;

	mm_checkpoint *ckp1 = model_checkpoint_take();

	for (unsigned i = 0; i < allocations_cnt; ++i) {
		for (unsigned j = 0; j < block_size/sizeof(uint64_t); ++j) {
			allocations[i][j] = lcg_random_u(rng_state);
		}
	}

	errs += __wrap_malloc(block_size) != NULL;

	mm_checkpoint *ckp2 = model_checkpoint_take();
	model_checkpoint_restore(ckp1);

	for (unsigned i = 0; i < allocations_cnt; ++i) {
		for (unsigned j = 0; j < block_size/sizeof(uint64_t); ++j) {
			errs += allocations[i][j] != lcg_random_u(rng_state_snap);
		}

		__wrap_free(allocations[i]);
	}

	model_checkpoint_restore(ckp2);

	errs += __wrap_malloc(block_size) != NULL;

	for (unsigned i = 0; i < allocations_cnt; ++i) {
		for (unsigned j = 0; j < block_size/sizeof(uint64_t); ++j) {
			errs += allocations[i][j] != lcg_random_u(rng_state_snap);
		}

		__wrap_free(allocations[i]);
	}

	free(allocations);
	model_checkpoint_free(ckp1);
	model_checkpoint_free(ckp2);
	return -(!!errs);
}

static int model_allocator_test(void)
{
	current_lp = malloc(sizeof(*current_lp));
	model_memory_lp_init();
	lcg_init(rng_state, (rid + 1) * 1713);

	unsigned errs = 0;
	for (unsigned j = B_BLOCK_EXP; j < B_TOTAL_EXP; ++j) {
		errs += block_size_test(j) < 0;
	}

	errs += __wrap_malloc(0) != NULL;
	errs += __wrap_calloc(0, sizeof(uint64_t)) != NULL;

	__wrap_free(NULL);

	uint64_t *mem = __wrap_calloc(1, sizeof(uint64_t));
	errs += *mem;
	__wrap_free(mem);

	model_memory_lp_fini();
	free(current_lp);
	return -(!!errs);
}

const struct _test_config_t test_config = {
	.test_name = "model allocator",
	.threads_count = 4,
	.test_fnc = model_allocator_test
};

