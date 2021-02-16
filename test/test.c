/**
 * @file test/test.c
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

#include <limits.h>
#include <memory.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>

#ifndef ROOTSIM_TEST_NAME
#define ROOTSIM_TEST_NAME "rs_test"
#endif

static char *t_out_buf;
static size_t t_out_buf_size;
static size_t t_out_wrote;

static char **test_argv;

__attribute__((weak)) lp_id_t n_lps;
#ifdef ROOTSIM_MPI
__attribute__((weak)) nid_t nid;
__attribute__((weak)) nid_t n_nodes = 1;
#endif
__attribute__((weak)) rid_t n_threads;
__attribute__((weak)) __thread rid_t rid;

__attribute__((weak)) int log_level;

__attribute__((weak))
void _log_log(int level, const char *file, unsigned line, const char *fmt, ...)
{
	(void) level;
	(void) file;
	(void) line;
	(void) fmt;
}

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
	free(t_out_buf);
}

/**
 * @brief The test wrapper which allows to intervene before the actual main()
 */
__attribute__((constructor))
void main_wrapper(void)
{
	int test_argc = 0;

	puts("Starting " ROOTSIM_TEST_NAME " test");

	n_threads = test_config.threads_count;

	atexit(test_atexit);

	if (init_arguments(&test_argc, &test_argv) == -1)
		exit(TEST_BAD_FAIL_EXIT_CODE);

	t_out_buf_size = 1;
	t_out_buf = malloc(t_out_buf_size);
	if (t_out_buf == NULL)
		exit(TEST_BAD_FAIL_EXIT_CODE);

	int test_ret = main(test_argc, test_argv);
	if (test_ret)
		exit(test_ret);

	if (t_out_wrote < test_config.expected_output_size) {
		puts("Test failed: output is shorter than the expected one");
		exit(-1);
	}

	puts("Successfully run " ROOTSIM_TEST_NAME " test");
	exit(0);
}

static int test_printf_internal(const char *restrict fmt, va_list args)
{
	va_list args_cpy;
	va_copy(args_cpy, args);
	size_t p_size = vsnprintf(t_out_buf, t_out_buf_size, fmt, args_cpy);
	va_end(args_cpy);

	if (t_out_wrote + p_size > test_config.expected_output_size) {
		puts("Test failed: output is longer than the expected one");
		exit(-1);
	}

	if (p_size >= t_out_buf_size) {
		do {
			t_out_buf_size *= 2;
		} while(p_size >= t_out_buf_size);

		free(t_out_buf);
		t_out_buf = malloc(t_out_buf_size);
		if (t_out_buf == NULL)
			exit(TEST_BAD_FAIL_EXIT_CODE);

		vsnprintf(t_out_buf, t_out_buf_size, fmt, args);
	}

	if (memcmp(t_out_buf, test_config.expected_output + t_out_wrote, p_size)) {
		printf("Test failed: output is different from the expected one %zu\n", t_out_wrote);
		exit(-1);
	}
	t_out_wrote += p_size;

	return p_size;
}

/**
 * @brief Registers a formatted string to compare against the expected output
 * @return the number of successfully registered characters
 */
int test_printf(const char *restrict fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int ret = test_printf_internal(fmt, args);
	va_end(args);
	return ret;
}

/**
 * @brief Registers a formatted string to compare against the expected output
 * @return the number of successfully registered characters
 *
 * Cloned definition needed because the LLVM plugin expects a suffixed symbol
 */
int test_printf_pr(const char *restrict fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int ret = test_printf_internal(fmt, args);
	va_end(args);
	return ret;
}

/**
 * @brief Synchronizes threads on a barrier
 * @return true if this thread has been elected as leader, false otherwise
 *
 * This is a more battle tested although less performing version of the thread
 * barrier. We can't rely on the pthread barrier because it's not portable.
 */
bool test_thread_barrier(void)
{
	static atomic_uint b_in, b_out, b_cr;

	unsigned i;
	unsigned count = test_config.threads_count;
	unsigned max_in_before_reset = (UINT_MAX / 2) - (UINT_MAX / 2) % count;
	do {
		i = atomic_fetch_add_explicit(
			&b_in, 1U, memory_order_acq_rel) + 1;
	} while (__builtin_expect(i > max_in_before_reset, 0));

	unsigned cr = atomic_load_explicit(&b_cr, memory_order_relaxed);

	bool leader = i == cr + count;
	if (leader) {
		atomic_store_explicit(&b_cr, cr + count, memory_order_release);
	} else {
		while (i > cr) {
			cr = atomic_load_explicit(&b_cr, memory_order_relaxed);
		}
	}
	atomic_thread_fence(memory_order_acquire);

	unsigned o = atomic_fetch_add_explicit(&b_out, 1, memory_order_release) + 1;
	if (__builtin_expect(o == max_in_before_reset, 0)) {
		atomic_thread_fence(memory_order_acquire);
		atomic_store_explicit(&b_cr, 0, memory_order_relaxed);
		atomic_store_explicit(&b_out, 0, memory_order_relaxed);
		atomic_store_explicit(&b_in, 0, memory_order_release);
	}
	return leader;
}
