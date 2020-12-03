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
 * @copyright
 * Copyright (C) 2008-2020 HPDCS Group
 * https://hpdcs.github.io
 *
 * This file is part of ROOT-Sim (ROme OpTimistic Simulator).
 *
 * ROOT-Sim is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; only version 3 of the License applies.
 *
 * ROOT-Sim is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ROOT-Sim; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef ROOTSIM_OPT_OPT
#define ROOTSIM_OPT_OPT "-O3 -flto"
#endif

// FIXME: we don't include absolute paths in code!
#ifndef ROOTSIM_CC
#define ROOTSIM_CC "/usr/bin/clang"
#endif

// FIXME: we don't include absolute paths in code!
#ifndef ROOTSIM_LIB_DIR
#define ROOTSIM_LIB_DIR "/usr/lib/"
#endif

// FIXME: we don't include absolute paths in code!
#ifndef ROOTSIM_INC_DIR
#define ROOTSIM_INC_DIR "/usr/include/"
#endif

static const char cmd_line_prefix[] =
	ROOTSIM_CC " "
	ROOTSIM_OPT_OPT " "
	"-I" ROOTSIM_INC_DIR " "
	"-lpthread "
	"-lm "
	"-Xclang -load "
	"-Xclang " ROOTSIM_LIB_DIR "librootsim-llvm.so "
	ROOTSIM_LIB_DIR "librootsim.a "
	ROOTSIM_LIB_DIR "librootsim-mods.a"
;

int main(int argc, char **argv)
{
	(void) argc;
	++argv;
	size_t tot_size = sizeof(cmd_line_prefix) - 1;
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

	if (system(cmd_line)) {
		free(cmd_line);
		fprintf(stderr, "Unable to run " ROOTSIM_CC);
		return -1;
	}
	free(cmd_line);
	return 0;
}

