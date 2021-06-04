/**
 * @file compiler.c
 *
 * @brief The ROOT-Sim compiler
 *
 * This is the ROOT-Sim compiler, a compiler wrapper which allows
 * to setup all necessary includes and configurations to run with
 * a parallel or distributed simulation. This is targeting low
 * level C models.
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>

#ifndef ROOTSIM_OPTIMIZATION_OPTIONS
/// The optimization options to be used when compiling models
/** This macro is filled in at build time */
#define ROOTSIM_OPTIMIZATION_OPTIONS ""
#endif

#ifndef ROOTSIM_CC
/// The path of the C compiler to use when compiling models
/** This macro is filled in at build time */
#define ROOTSIM_CC ""
#endif

#ifndef ROOTSIM_LIB_DIR
/// The path of the installed ROOT-Sim libraries
/** This macro is filled in at build time */
#define ROOTSIM_LIB_DIR ""
#endif

#ifndef ROOTSIM_INC_DIR
/// The path of the installed ROOT-Sim headers
/** This macro is filled in at build time */
#define ROOTSIM_INC_DIR ""
#endif

static const char *cmd_line_plugin = "-fplugin=%s/librootsim-cc_llvm.so ";
static const char *cmd_line_libraries = "-Wl,--as-needed -lrootsim -lrootsim-mods -lm -lpthread";

/// The path to the ROOT-Sim include directory. Can be overridden with -rsinc at command line
const char *include_path = ROOTSIM_INC_DIR;
/// The path to the ROOT-Sim library path. Can be overridden with -rslib at command line
const char *lib_path = ROOTSIM_LIB_DIR;

/**
 * @brief A simple dynamic string data structure to assemble command line arguments
 */
struct dynstr {
	char *buff; ///< The current string
	size_t size; ///< The total number of available characters
	size_t used; ///< The current number of used characters
};

/// The dynamic string to build the compiling command line
struct dynstr command_line;

/// The dynamic string to keep user-specified arguments
struct dynstr user_args;

/**
 * @brief A version of strcat() working on struct dynstr
 * @param cmd The dynamic string where to perform the cat
 * @param src The string to cat
 * @param len The length of the string to cat
 */
static void cmdline_strcat(struct dynstr *cmd, const char *src, size_t len)
{
	while(cmd->used + len >= cmd->size) {
		cmd->size *= 2;
		cmd->buff = realloc(cmd->buff, cmd->size);
	}

	strncat(cmd->buff, src, len);
	cmd->used += len;
}

/**
 * @brief Function to assemble a string to append to a dynamic string
 *
 * @param cmd The dynamic string to append the generated string to
 * @param fmt The format string (printf-like)
 * @param ... A variable number of arguments
 */
static void cmdline_printcat(struct dynstr *cmd, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	size_t len = vsnprintf(NULL, 0, fmt, args);
	va_end(args);

	char buffer[len + 1];

	va_start(args, fmt);
	vsprintf(buffer, fmt, args);
	va_end(args);

	cmdline_strcat(cmd, buffer, len);
}

/**
 * @brief Initialize a dynamic string
 * @param cmd The dynamic string to initialize
 * @param len The initial size of the buffer
 */
static void cmdline_init(struct dynstr *cmd, size_t len)
{
	assert(len > 0);
	cmd->buff = malloc(len);
	cmd->buff[0] = '\0';
	cmd->size = len;
	cmd->used = 1;

}

/**
 * @brief The main entry point of the custom compiler
 * @param argc The count of command line arguments, as per ISO C standard
 * @param argv The list of command line arguments, as per ISO C standard
 */
int main(int argc, char **argv)
{
	(void)argc;
	int ret = 0;
	++argv;

	cmdline_init(&command_line, 512);
	cmdline_init(&user_args, 64);

	while (*argv) {
		if(strcmp(*argv, "-rsinc") == 0) {
			include_path = *(++argv);
			++argv;
			continue;
		}
		if(strcmp(*argv, "-rslib") == 0) {
			lib_path = *(++argv);
			++argv;
			continue;
		}

		cmdline_strcat(&user_args, *argv, strlen(*argv));
		cmdline_strcat(&user_args, " ", 1);
		++argv;
	}

	// Assemble the final command
	cmdline_printcat(&command_line, "%s %s -I%s -L%s ", ROOTSIM_CC, ROOTSIM_OPTIMIZATION_OPTIONS, include_path, lib_path);
	cmdline_printcat(&command_line, cmd_line_plugin, lib_path);
	cmdline_strcat(&command_line, user_args.buff, user_args.used);
	cmdline_strcat(&command_line, cmd_line_libraries, strlen(cmd_line_libraries));

	if (system(command_line.buff)) {
#if LOG_LEVEL <= LOG_DEBUG
		fprintf(stderr, "Failed to run: %s\n", command_line.buff);
#else
		fprintf(stderr, "Failed to run: " ROOTSIM_CC "\n");
#endif
		ret = -1;
	}

	free(user_args.buff);
	free(command_line.buff);
	return ret;
}
