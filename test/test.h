#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

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
