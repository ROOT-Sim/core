#include <test.h>

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

FILE *test_output_file;
static pthread_barrier_t t_barrier;
static char **test_argv;
static char *test_output;

struct stub_arguments {
	int (*test_fnc)(unsigned);
	unsigned tid;
};

__attribute__((weak)) void* __real_malloc(size_t mem_size);
__attribute__((weak)) void __real_free(void *ptr);

void* test_malloc(size_t mem_size)
{
	return __real_malloc ? __real_malloc(mem_size) : malloc(mem_size);
}

void test_free(void *ptr)
{
	__real_free ? __real_free(ptr) : free(ptr);
}


int __real_main(int argc, char **argv);

static void* test_run_stub(void* arg)
{
	struct stub_arguments *args = arg;
	int ret = args->test_fnc(args->tid);
	pthread_exit((void *)(intptr_t)ret);
}

__attribute__((weak)) int main(int argc, char **argv)
{
	(void)argc; (void)argv;
	int ret = 0;
	pthread_t threads[test_config.threads_count];
	struct stub_arguments args[test_config.threads_count];

	if(test_config.test_init_fnc && (ret = test_config.test_init_fnc()))
		return ret;

	for(unsigned i = 0; i < test_config.threads_count; ++i){
		args[i].test_fnc = test_config.test_fnc;
		args[i].tid = i;
		pthread_create(&threads[i], NULL, test_run_stub, &args[i]);
	}

	for(unsigned i = 0; i < test_config.threads_count; ++i){
		void* ret_r = NULL;
		pthread_join(threads[i], &ret_r);
		if((intptr_t)ret_r < ret)
			ret = (int)(intptr_t)ret_r;
	}

	if(test_config.test_fini_fnc){
		int aux = test_config.test_fini_fnc();
		ret = ret < aux ? ret : aux;
	}
	return ret;
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
	int ret = TEST_FAIL_EXIT_CODE;
	int test_ret = -1;
	int test_argc = 0;
	size_t test_output_size = 0;

	printf("Starting %s test\n", test_config.test_name);

	atexit(test_atexit);

	if(pthread_barrier_init(&t_barrier, NULL,
		test_config.threads_count ? test_config.threads_count : 1))
		goto out;

	if(init_arguments(&test_argc, &test_argv) == -1)
		goto out;

	test_output_file = open_memstream(&test_output, &test_output_size);
	if(test_output_file == NULL)
		goto out;

	test_ret = __real_main(test_argc, test_argv);
	if(test_ret < 0){
		ret = test_ret;
		goto out;
	}

	if(fflush(test_output_file) == EOF)
		goto out;

	if(test_config.expected_output && (
			test_output_size != test_config.expected_output_size ||
			memcmp(test_output, test_config.expected_output, test_output_size)
		)
	){
		printf("%s", test_output);
		goto out;
	}

	printf("Successfully run %s test\n", test_config.test_name);
	ret = test_ret;

	out:
	return ret;
}

int test_thread_barrier(void)
{
	return pthread_barrier_wait(&t_barrier) == PTHREAD_BARRIER_SERIAL_THREAD;
}
