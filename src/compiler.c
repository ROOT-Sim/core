#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <memory.h>
#include <sys/wait.h>

#define GCC_CMD "/usr/bin/gcc"

extern char **environ;

static const char * const additional_args[] = {
	"-lm",
	NULL
};

static int child_proc(int argc, char **argv, char **environment)
{
	char **new_argv = malloc(sizeof(*new_argv) * argc + sizeof(additional_args));
	memcpy(new_argv, argv, sizeof(*new_argv) * argc);
	memcpy(new_argv + argc, additional_args, sizeof(additional_args));

	unsigned i = 0;
	while(environment[i])
		puts(environment[i++]);
	i = 0;
	while(new_argv[i])
		puts(new_argv[i++]);

	if(execve(GCC_CMD, argv, environment)){
		fprintf(stderr, "Unable to run " GCC_CMD);
		return -1;
	}
	return 0;
}

int main(int argc, char **argv)
{
	pid_t child_pid;
	if(!(child_pid = fork())){
		return child_proc(argc, argv, environ);
	}
	int child_status = -1;
	while(wait(&child_status) != child_pid){
		sleep(1);
	}
}

