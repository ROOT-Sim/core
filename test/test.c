#include <test.h>

#include <limits.h>
#include <memory.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>

#include <arch/arch.h>

struct stub_arguments {
	int (*test_fnc)(void);
	unsigned tid;
};

static char *t_out_buf;
static size_t t_out_buf_size;
static size_t t_out_wrote;

static char **test_argv;

__attribute__((weak)) lp_id_t n_lps;
__attribute__((weak)) nid_t n_nodes = 1;
__attribute__((weak)) rid_t n_threads;
__attribute__((weak)) nid_t nid;
__attribute__((weak)) __thread rid_t rid;

__attribute__((weak)) void* __real_malloc(size_t mem_size);
__attribute__((weak)) void __real_free(void *ptr);

void *test_malloc(size_t mem_size)
{
	if (__real_malloc)
		return __real_malloc(mem_size);
	else
		return malloc(mem_size);
}

void test_free(void *ptr)
{
	if (__real_free)
		__real_free(ptr);
	else
		free(ptr);
}

static arch_thr_ret_t ARCH_CALL_CONV test_run_stub(void *arg)
{
	struct stub_arguments *args = arg;
	rid = args->tid;
	int ret = args->test_fnc();
	return ret ? ARCH_THR_RET_FAILURE : ARCH_THR_RET_SUCCESS;
}

int __real_main(int argc, char **argv);
int __attribute__((weak, alias ("test_main"))) main(int argc, char **argv);

int test_main(int argc, char **argv)
{
	(void)argc; (void)argv;
	int ret = 0;
	arch_thr_t threads[test_config.threads_count];
	struct stub_arguments args[test_config.threads_count];

	if(test_config.test_init_fnc && (ret = test_config.test_init_fnc())){
		printf("Test initialization failed with code %d\n", ret);
		return ret;
	}

	for(unsigned i = 0; i < test_config.threads_count; ++i){
		args[i].test_fnc = test_config.test_fnc;
		args[i].tid = i;
		if (arch_thread_create(&threads[i], test_run_stub, &args[i])) {
			return TEST_BAD_FAIL_EXIT_CODE;
		}
	}

	for(unsigned i = 0; i < test_config.threads_count; ++i){
		arch_thr_ret_t thr_ret;
		if (arch_thread_wait(threads[i], &thr_ret)) {
			return TEST_BAD_FAIL_EXIT_CODE;
		}
		if (thr_ret) {
			printf("Thread %u failed the test\n", i);
			return -1;
		}
	}

	if(test_config.test_fini_fnc && (ret = test_config.test_fini_fnc())){
		printf("Test finalization failed with code %d\n", ret);
		return ret;
	}

	return 0;
}

static int init_arguments(int *argc_p, char ***argv_p)
{
	int argc = 0;
	if(test_config.test_arguments){
		while(test_config.test_arguments[argc]){
			argc++;
		}
	}
	++argc;

	char **argv = test_malloc(sizeof(*argv) * (argc + 1));
	if(argv == NULL)
		return -1;

	argv[0] = "rootsim_test";

	if(test_config.test_arguments){
		memcpy(
			&argv[1],
			test_config.test_arguments,
			sizeof(*argv) * argc
		);
	} else {
		argv[1] = NULL;
	}

	*argc_p = argc;
	*argv_p = argv;
	return 0;
}

static void test_atexit(void)
{
	test_free(test_argv);
	test_free(t_out_buf);
}

int __wrap_main(void)
{
	int test_argc = 0;

	printf("Starting %s test\n", test_config.test_name);

	n_threads = test_config.threads_count ? test_config.threads_count : 1;

	atexit(test_atexit);

	if(init_arguments(&test_argc, &test_argv) == -1)
		return TEST_BAD_FAIL_EXIT_CODE;

	t_out_buf_size = 1;
	t_out_buf = test_malloc(t_out_buf_size);
	if (t_out_buf == NULL)
		return TEST_BAD_FAIL_EXIT_CODE;

	int test_ret = __real_main(test_argc, test_argv);
	if (test_ret) {
		return test_ret;
	}

	if (t_out_wrote < test_config.expected_output_size) {
		puts("Test failed: output is shorter than the expected one");
		exit(-1);
	}

	printf("Successfully run %s test\n", test_config.test_name);
	return 0;
}


int test_printf(const char *restrict fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	size_t p_size = vsnprintf(t_out_buf, t_out_buf_size, fmt, args);
	va_end(args);

	if (t_out_wrote + p_size > test_config.expected_output_size) {
		puts("Test failed: output is longer than the expected one");
		exit(-1);
	}

	if (p_size >= t_out_buf_size) {

		do {
			t_out_buf_size *= 2;
		} while(p_size >= t_out_buf_size);

		test_free(t_out_buf);
		t_out_buf = test_malloc(t_out_buf_size);
		if (t_out_buf == NULL)
			return TEST_BAD_FAIL_EXIT_CODE;

		va_start(args, fmt);
		vsnprintf(t_out_buf, t_out_buf_size, fmt, args);
		va_end(args);
	}

	if (memcmp(t_out_buf, test_config.expected_output + t_out_wrote, p_size)) {
		puts("Test failed: output is different from the expected one");
		exit(-1);
	}
	t_out_wrote += p_size;

	return p_size;
}

bool test_thread_barrier(void)
{
	static atomic_uint b_in, b_out, b_cr;

	unsigned i;
	unsigned count = test_config.threads_count;
	unsigned max_in_before_reset = (UINT_MAX/2) - (UINT_MAX/2) % count;
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
