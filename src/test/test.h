#pragma once

#include <stddef.h>
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>

#define TEST_FAIL_EXIT_CODE 99

extern struct _test_config_t {
	int (*test_init_fnc)(void);
	int (*test_fini_fnc)(void);
	int (*test_fnc)(unsigned);
	unsigned threads_count;
	const char *expected_output;
	size_t expected_output_size;
	const char *test_name;
	const char **test_arguments;
} test_config;

extern FILE *test_output_file;
