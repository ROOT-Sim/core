#include <test.h>

#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

FILE *test_output_file;

struct stub_arguments {
	int (*test_fnc)(unsigned);
	unsigned tid;
};

int __real_main(int argc, char **argv);

static void* test_run_stub(void* arg)
{
	struct stub_arguments *args = arg;
	int ret = args->test_fnc(args->tid);
	return (void *)(intptr_t)ret;
}

int __attribute__((weak)) main(int argc, char **argv)
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

	if(test_config.test_fini_fnc)
		ret = test_config.test_fini_fnc();

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

	char **argv = malloc(sizeof(*argv) * (argc + 1));
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

int __wrap_main(void)
{
	int ret = TEST_FAIL_EXIT_CODE;
	int test_ret = -1;
	int test_argc = 0;
	char **test_argv = NULL;
	char *test_output = NULL;
	size_t test_output_size = 0;

	printf("Starting %s test\n", test_config.test_name);

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
	)
		goto out;

	printf("Successfully run %s test\n", test_config.test_name);
	ret = test_ret;

	out:
	free(test_argv);
	if(test_output_file)
		fclose(test_output_file);
	free(test_output);

	return ret;
}
