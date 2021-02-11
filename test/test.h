/**
 * @file test/test.h
 *
 * @brief Test framework header
 *
 * The header of the minimal test framework used in the code base tests
 *
 * @copyright
 * Copyright (C) 2008-2020 HPDCS Group
 * https://hpdcs.github.io
 *
 * This file is part of ROOT-Sim (ROme OpTimistic Simulator).
 *
 * ROOT-Sim is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; only version 3 of the License applies.
 *
 * ROOT-Sim is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ROOT-Sim; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define TEST_BAD_FAIL_EXIT_CODE 99

extern const struct _test_config_t {
	int (*test_init_fnc)(void);
	int (*test_fini_fnc)(void);
	int (*test_fnc)(void);
	unsigned threads_count;
	const char *expected_output;
	size_t expected_output_size;
	const char **test_arguments;
} test_config;

extern int test_printf_br(const char *restrict fmt, ...);
extern int test_printf(const char *restrict fmt, ...);
extern bool test_thread_barrier(void);

// core.c mock
typedef uint64_t lp_id_t;
typedef unsigned rid_t;
typedef int nid_t;

extern lp_id_t n_lps;
extern nid_t n_nodes;
extern rid_t n_threads;
extern nid_t nid;
extern __thread rid_t rid;
