/**
 * @file test/datatypes/retractable_heap.c
 *
 * @brief Test: bitmap datatype
 *
 * SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <datatypes/retractable_heap.h>

#include <test.h>

#include <limits.h>

#define test_cmp_f(a, b) ((a).val < (b).val)
#define test_upd_f(e, p) ((e).pos = p)

#define NUM_ELEMENTS 16384
#define NUM_PRIORITY_CHANGES 256

struct test_elem {
	int val;
	array_count_t pos;
};

static retractable_heap_declare(struct test_elem) test_heap;

bool check_heap_property(void)
{
	array_count_t n = array_count(test_heap);
	struct test_elem *items = array_items(test_heap);
	for(array_count_t i = 0; i < n; ++i) {
		array_count_t c = retractable_heap_child_i(i);
		for(int k = 0; k < HEAP_K; k++) {
			if(c + k < n && !test_cmp_f(items[i], items[c + k]) && items[i].val != items[c + k].val)
				return false;
		}
	}
	return true;
}

static int retractable_heap_init_test(_unused void *unused)
{
	retractable_heap_init(test_heap);
	for(int i = 0; i < NUM_ELEMENTS; ++i) {
		struct test_elem e = (struct test_elem){.val = (int)test_random_range(INT_MAX / 2)};
		retractable_heap_insert(test_heap, test_cmp_f, test_upd_f, e);
		test_assert(check_heap_property());
	}
	return 0;
}

static int retractable_heap_priority_increase_test(_unused void *unused)
{
	for(int i = 0; i < NUM_PRIORITY_CHANGES; ++i) {
		test_assert(array_count(test_heap));

		array_count_t idx = test_random_range(array_count(test_heap) - 1);
		struct test_elem e = array_get_at(test_heap, idx);

		int delta = (int)test_random_range(64);
		if(e.val - delta < 0)
			continue;
		e.val -= delta;

		retractable_heap_priority_increased(test_heap, test_cmp_f, test_upd_f, e, idx);
		test_assert(check_heap_property());
	}
	return 0;
}

static int retractable_heap_priority_decrease_test(_unused void *unused)
{
	for(int i = 0; i < NUM_PRIORITY_CHANGES; ++i) {
		test_assert(array_count(test_heap));

		array_count_t idx = test_random_range(array_count(test_heap) - 1);
		struct test_elem e = array_get_at(test_heap, idx);

		int delta = (int)test_random_range(128);
		e.val += delta;

		retractable_heap_priority_decreased(test_heap, test_cmp_f, test_upd_f, e, idx);
		test_assert(check_heap_property());
	}
	return 0;
}

static int retractable_heap_fini_test(_unused void *unused)
{
	int min_val = retractable_heap_min(test_heap).val, n_elems = 0;
	while(!retractable_heap_is_empty(test_heap)) {
		int current_min = retractable_heap_extract(test_heap, test_cmp_f, test_upd_f).val;
		test_assert(current_min >= min_val);
		min_val = current_min;
		test_assert(check_heap_property());
		++n_elems;
	}
	test_assert(n_elems == NUM_ELEMENTS);

	retractable_heap_fini(test_heap);
	return 0;
}

int main(void)
{
	test("Testing retractable heap init", retractable_heap_init_test, NULL);
	test("Testing retractable heap priority increase", retractable_heap_priority_increase_test, NULL);
	test("Testing retractable heap priority decrease", retractable_heap_priority_decrease_test, NULL);
	test("Testing retractable heap fini", retractable_heap_fini_test, NULL);
	return 0;
}
