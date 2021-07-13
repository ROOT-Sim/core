/**
 * @file test/test.h
 *
 * @brief Test framework header
 *
 * The header of the minimal test framework used in the code base tests
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/// The exit code of tests when something fails horribly
/** Triggers an unconditional failure, even for expected-failure tests */
#define TEST_BAD_FAIL_EXIT_CODE 99

/// A complete test configuration
struct test_config {
	/// The test initialization function
	/** The return value is used as failure exit code if it is non-zero */
	int (*test_init_fnc)(void);
	/// The test finalization function
	/** The return value is used as failure exit code if it is non-zero */
	int (*test_fini_fnc)(void);
	/// The core test function
	/** The return value is used as failure exit code if it is non-zero. */
	int (*test_fnc)(void);
	/// @a test_fnc is executed with that many cores
	unsigned threads_count;
	/// The command line arguments passed to the wrapped main function
	const char **test_arguments;
};

/// The test configuration object, must be defined by the test sources
extern const struct test_config test_config;
extern bool test_thread_barrier(void);

// core.h typedefs
typedef uint64_t lp_id_t;
typedef unsigned rid_t;
typedef int nid_t;
