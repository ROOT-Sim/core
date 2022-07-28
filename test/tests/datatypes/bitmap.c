/**
 * @file test/tests/datatypes/bitmap_test.c
 *
 * @brief Test: bitmap datatype
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <memory.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include "test.h"

#include "datatypes/bitmap.h"

#define THREAD_REPS 100000
#define BITMAP_ENTRIES 10000

static int bitmap_test(_unused void *_)
{
	int ret = 0;

	size_t b_size = bitmap_required_size(BITMAP_ENTRIES);
	block_bitmap *b = malloc(b_size);
	bitmap_initialize(b, BITMAP_ENTRIES);

	bool *b_check = malloc(sizeof(*b_check) * BITMAP_ENTRIES);
	memset(b_check, 0, sizeof(*b_check) * BITMAP_ENTRIES);

	unsigned i = THREAD_REPS;
	while (i--) {
		unsigned e = test_random_range(BITMAP_ENTRIES);
		bool v = (double)test_random_u() / RAND_MAX > 0.5;
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

int main(void)
{
	test("Testing bitmap implementation", bitmap_test, NULL);
}
