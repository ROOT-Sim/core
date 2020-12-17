#include <test.h>

#include <limits.h>
#include <memory.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>

#include "../src/arch/thread.h"

#ifndef ROOTSIM_TEST_NAME
#define ROOTSIM_TEST_NAME "rs_test"
#endif

static char *t_out_buf;
static size_t t_out_buf_size;
static size_t t_out_wrote;

static char **test_argv;

__attribute__((weak)) lp_id_t n_lps;
__attribute__((weak)) nid_t n_nodes = 1;
__attribute__((weak)) rid_t n_threads;
__attribute__((weak)) nid_t nid;
__attribute__((weak)) __thread rid_t rid;

int main(int argc, char **argv);

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

static void test_atexit(void)
{
	free(test_argv);
	free(t_out_buf);
}

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
			return TEST_BAD_FAIL_EXIT_CODE;

		vsnprintf(t_out_buf, t_out_buf_size, fmt, args);
	}

	if (memcmp(t_out_buf, test_config.expected_output + t_out_wrote, p_size)) {
		printf("Test failed: output is different from the expected one %lu\n", t_out_wrote);
		exit(-1);
	}
	t_out_wrote += p_size;
	return p_size;
}

int test_printf_pr(const char *restrict fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int ret = test_printf_internal(fmt, args);
	va_end(args);

	return ret;
}

int test_printf(const char *restrict fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int ret = test_printf_internal(fmt, args);
	va_end(args);

	return ret;
}

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
			cr = atomic_load_explicit (&b_cr, memory_order_relaxed);
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
