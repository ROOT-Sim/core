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

static const char cmd_line_prefix[] =
	ROOTSIM_CC " "
	ROOTSIM_OPTIMIZATION_OPTIONS " "
	"-I" ROOTSIM_INC_DIR " "
	"-fuse-ld=lld "
	"-Xclang -load "
	"-Xclang " ROOTSIM_LIB_DIR "librootsim-llvm.so"
;
static const char cmd_line_suffix[] =
	" -Wl,--as-needed "
	ROOTSIM_LIB_DIR "librootsim.a "
	ROOTSIM_LIB_DIR "librootsim-mods.a "
	"-lm "
	"-lpthread"
;

/**
 * @brief The main entry point of the custom compiler
 * @param argc The count of command line arguments, as per ISO C standard
 * @param argv The list of command line arguments, as per ISO C standard
 */
int main(int argc, char **argv)
{
	(void) argc;
	++argv;
	size_t tot_size = sizeof(cmd_line_prefix) + sizeof(cmd_line_suffix) - 1;
	char **argv_tmp = argv;
	while (*argv_tmp) {
		tot_size += strlen(*argv_tmp) + 1;
		++argv_tmp;
	}

	char *cmd_line = malloc(tot_size);
	if (cmd_line == NULL) {
		fprintf(stderr, "Unable to allocate memory!");
		return -1;
	}

	char *ptr = cmd_line;
	memcpy(ptr, cmd_line_prefix, sizeof(cmd_line_prefix) - 1);
	ptr += sizeof(cmd_line_prefix) - 1;

	while (*argv) {
		*ptr = ' ';
		++ptr;

		size_t l = strlen(*argv);
		memcpy(ptr, *argv, l);
		ptr += l;

		++argv;
	}

	memcpy(ptr, cmd_line_suffix, sizeof(cmd_line_suffix));

	if (system(cmd_line)) {
		free(cmd_line);
		fprintf(stderr, "Unable to run " ROOTSIM_CC "\n");
		return -1;
	}
	free(cmd_line);
	return 0;
}

