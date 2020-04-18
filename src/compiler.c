#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>

#ifndef CC_CMD
#define CC_CMD "/usr/bin/gcc"
#endif

#ifndef NEUROME_LIB_DIR
#define NEUROME_LIB_DIR "/usr/lib/"
#endif

#ifndef NEUROME_INC_DIR
#define NEUROME_INC_DIR "/usr/include/"
#endif

static const char *add_args_serial = {
	"-o "
	"model_serial "
	"-O3 "
	"-I"NEUROME_INC_DIR" "
	NEUROME_LIB_DIR"libneurome-serial.a "
	"-lm"
};

static const char *add_args_parallel = {
	"-o "
	"model_parallel "
	"-O3 "
	"-I"NEUROME_INC_DIR" "
	NEUROME_LIB_DIR"libneurome-parallel.a "
	"-Wl,--wrap=malloc,--wrap=realloc,--wrap=free,--wrap=calloc "
	"-lpthread "
	"-lm"
};

static int child_proc(int argc, char **argv, const char *add_args)
{
	size_t tot_size = strlen(CC_CMD) + argc + strlen(add_args) + 1;
	unsigned i = 1;
	while(argv[i]){
		tot_size += strlen(argv[i]);
		++i;
	}

	char *cmd_line = malloc(tot_size);
	if (cmd_line == NULL){
		fprintf(stderr, "Unable to allocate memory!");
		return -1;
	}

	char *ptr = cmd_line;
	strcpy(ptr, CC_CMD);
	ptr += strlen(CC_CMD);
	*ptr = ' ';
	++ptr;

	i = 1;
	while(argv[i]){
		strcpy(ptr, argv[i]);
		ptr += strlen(argv[i]);
		*ptr = ' ';
		++ptr;
		++i;
	}

	strcpy(ptr, add_args);

	if (system(cmd_line)) {
		fprintf(stderr, "Unable to run " CC_CMD);
		return -1;
	}
	return 0;
}

int main(int argc, char **argv)
{
	pid_t this_pid;
	int s_stat = -1, p_stat = -1;
	if (!(this_pid = fork())) {
		return child_proc(argc, argv, add_args_serial);
	}

	while (waitpid(this_pid, &s_stat, 0) != this_pid) {
		sleep(1);
	}

	if (!(this_pid = fork())) {
		return child_proc(argc, argv, add_args_parallel);
	}

	while (waitpid(this_pid, &p_stat, 0) != this_pid) {
		sleep(1);
	}

	return s_stat + p_stat;
}

