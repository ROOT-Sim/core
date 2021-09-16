/**
 * @file test/datatypes/bitmap_test.c
 *
 * @brief Test: bitmap datatype
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <test.h>
#include <test_rng.h>

#include <datatypes/bitmap.h>

#include <memory.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>

#define THREAD_CNT 6
#define THREAD_REPS 100000
#define BITMAP_ENTRIES 10000

__thread rid_t rid;

static int bitmap_test(void)
{
	int ret = 0;
	test_rng_state rng_state;
	lcg_init(rng_state, (rid + 1) * 1713);

	size_t b_size = bitmap_required_size(BITMAP_ENTRIES);
	block_bitmap *b = malloc(b_size);
	bitmap_initialize(b, BITMAP_ENTRIES);

	bool *b_check = malloc(sizeof(*b_check) * BITMAP_ENTRIES);
	memset(b_check, 0, sizeof(*b_check) * BITMAP_ENTRIES);

	unsigned i = THREAD_REPS;
	while (i--) {
		unsigned e = lcg_random_u(rng_state) % BITMAP_ENTRIES;
		bool v = lcg_random(rng_state) > 0.5;
		if (v)
			bitmap_set(b, e);
		else
			bitmap_reset(b, e);

		b_check[e] = v;
	}

	unsigned c = bitmap_count_set(b, b_size);

	i = BITMAP_ENTRIES;
	while (i--)
		c -= b_check[i];

	if (c)
		return -1;

#define bitmap_check_test(i) 	\
__extension__({			\
	if (!b_check[i])	\
		return -1;	\
	b_check[i] = false;	\
})

	bitmap_foreach_set(b, b_size, bitmap_check_test);

	free(b);
	free(b_check);
	return ret;
}

const struct test_config test_config = {
	.threads_count = THREAD_CNT,
	.test_fnc = bitmap_test
};
