/**
 * @file test/test_framework.c
 *
 * @brief Test framework source
 *
 * The source of the minimal test framework used in the code base tests
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <test.h>

#include <arch/thread.h>

#include <memory.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>

#ifndef ROOTSIM_TEST_NAME
#define ROOTSIM_TEST_NAME "rs_test"
#endif

static char **test_argv;

int main(int argc, char **argv);

/**
 * @brief Initializes ISO C compliant argc and argv from the test configuration
 * @param argc_p a pointer to a variable which will hold the computed argc value
 * @param argv_p a pointer to a variable which will hold the computer argv value
 * @return 0 in case of success, -1 in case of failure
 */
static int init_arguments(int *argc_p, char ***argv_p)
{
	int argc = 0;
	if (test_config.test_arguments) {
		while (test_config.test_arguments[argc]) {
			++argc;
		}
	}
	++argc;

	char **argv = malloc(sizeof(*argv) * (argc + 1));
	if(argv == NULL)
		return -1;

	argv[0] = ROOTSIM_TEST_NAME;

	if (test_config.test_arguments) {
		memcpy(&argv[1], test_config.test_arguments,
		       sizeof(*argv) * argc);
	} else {
		argv[1] = NULL;
	}

	*argc_p = argc;
	*argv_p = argv;
	return 0;
}

/**
 * @brief The exit handler, to exit cleanly even in case of errors
 */
static void test_atexit(void)
{
	free(test_argv);
}

/**
 * @brief The test wrapper which allows to intervene before the actual main()
 */
__attribute__((constructor))
void main_wrapper(void)
{
	int test_argc = 0;

	puts("Starting " ROOTSIM_TEST_NAME " test");

	extern rid_t n_threads;
	n_threads = test_config.threads_count;

	atexit(test_atexit);

	if (init_arguments(&test_argc, &test_argv) == -1)
		exit(TEST_BAD_FAIL_EXIT_CODE);

	int test_ret = main(test_argc, test_argv);
	if (!test_ret)
		puts("Successfully run " ROOTSIM_TEST_NAME " test");

	exit(test_ret);
}
