#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <memory.h>
#include <sys/wait.h>

#ifndef CC_CMD
#define CC_CMD "/usr/bin/gcc"
#endif

#ifndef NEUROME_LIBS_PATH
#define NEUROME_LIBS_PATH "/usr/lib/"
#endif

static const char *const additional_args[] = {
	"-lm",
	NULL
};

static int child_proc(int argc, char **argv)
{
	char **new_argv = malloc(
		sizeof(*new_argv) * argc + sizeof(additional_args));
	memcpy(new_argv, argv, sizeof(*new_argv) * argc);
	memcpy(new_argv + argc, additional_args, sizeof(additional_args));

	argv[0] = CC_CMD;

	if (execv(CC_CMD, argv)) {
		fprintf(stderr, "Unable to run " CC_CMD);
		return -1;
	}
	return 0;
}

int main(int argc, char **argv)
{
	pid_t child_pid;
	if (!(child_pid = fork())) {
		return child_proc(argc, argv);
	}
	int child_status = -1;
	while (wait(&child_status) != child_pid) {
		sleep(1);
	}
	return child_status;
}

