#include <test.h>

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

FILE *test_output_file;
static pthread_barrier_t t_barrier;
static char **test_argv;
static char *test_output;

__attribute__((weak)) lp_id_t n_lps;
__attribute__((weak)) nid_t n_nodes = 1;
__attribute__((weak)) rid_t n_threads;
__attribute__((weak)) nid_t nid;
__attribute__((weak)) __thread rid_t rid;

struct stub_arguments {
	int (*test_fnc)(void);
	unsigned tid;
};

__attribute__((weak)) void* __real_malloc(size_t mem_size);
__attribute__((weak)) void __real_free(void *ptr);

void* test_malloc(size_t mem_size)
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

static void* test_run_stub(void* arg)
{
	struct stub_arguments *args = arg;
	rid = args->tid;
	int ret = args->test_fnc();
	return (void *)(intptr_t)ret;
}

int __real_main(int argc, char **argv);
int __attribute__((weak, alias ("test_main"))) main(int argc, char **argv);

int test_main(int argc, char **argv)
{
	(void)argc; (void)argv;
	int ret = 0;
	pthread_t threads[test_config.threads_count];
	struct stub_arguments args[test_config.threads_count];

	if(test_config.test_init_fnc && (ret = test_config.test_init_fnc())){
		printf("Test initialization failed with code %d\n", ret);
		return ret;
	}

	for(unsigned i = 0; i < test_config.threads_count; ++i){
		args[i].test_fnc = test_config.test_fnc;
		args[i].tid = i;
		if(pthread_create(&threads[i], NULL, test_run_stub, &args[i])) {
			return TEST_BAD_FAIL_EXIT_CODE;
		}
	}

	for(unsigned i = 0; i < test_config.threads_count; ++i){
		void *ret_tv = NULL;
		if(pthread_join(threads[i], &ret_tv)) {
			return TEST_BAD_FAIL_EXIT_CODE;
		}
		if((ret = (intptr_t)ret_tv)) {
			printf("Thread %u failed the test with code %d\n", i, ret);
			return ret;
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

	argv[0] = "neurome_test";

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
	if(test_output_file)
		fclose(test_output_file);
	test_free(test_output);
	pthread_barrier_destroy(&t_barrier);
}

int __wrap_main(void)
{
	int test_argc = 0;
	size_t test_output_size = 0;

	printf("Starting %s test\n", test_config.test_name);

	n_threads = test_config.threads_count ? test_config.threads_count : 1;

	atexit(test_atexit);

	if(pthread_barrier_init(&t_barrier, NULL, n_threads))
		return TEST_BAD_FAIL_EXIT_CODE;

	if(init_arguments(&test_argc, &test_argv) == -1)
		return TEST_BAD_FAIL_EXIT_CODE;

	test_output_file = open_memstream(&test_output, &test_output_size);
	if(test_output_file == NULL)
		return TEST_BAD_FAIL_EXIT_CODE;

	int test_ret = __real_main(test_argc, test_argv);
	if(test_ret){
		return test_ret;
	}

	if(fflush(test_output_file) == EOF)
		return TEST_BAD_FAIL_EXIT_CODE;

	if(test_config.expected_output && (
			test_output_size != test_config.expected_output_size ||
			memcmp(test_output, test_config.expected_output, test_output_size)
		)
	){
		printf("Test failed: output is different from the expected one\n");
		printf("%s", test_output);
		return 1;
	}

	printf("Successfully run %s test\n", test_config.test_name);
	return 0;
}

int test_thread_barrier(void)
{
	return pthread_barrier_wait(&t_barrier) == PTHREAD_BARRIER_SERIAL_THREAD;
}
