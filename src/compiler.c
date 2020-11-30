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

#ifdef NDEBUG
#define OPTIMIZATION_OPTIONS "-O3 -flto "
#else
#define OPTIMIZATION_OPTIONS "-g -O0 "
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

static const char *add_args_serial =
	"-o "
	"model_serial "
	OPTIMIZATION_OPTIONS
	"-I"ROOTSIM_INC_DIR" "
	ROOTSIM_LIB_DIR"librootsim-serial.a "
	"-lm"
;

static const char *add_args_parallel =
	"-o "
	"model_parallel "
	OPTIMIZATION_OPTIONS
	"-I"ROOTSIM_INC_DIR" "
	ROOTSIM_LIB_DIR"librootsim-parallel.a "
	"-lpthread "
	"-lm "
	"-Xclang -load "
	"-Xclang "ROOTSIM_LIB_DIR"librootsim-llvm.so"
;

static int child_proc(int argc, char **argv, const char *add_args)
{
	size_t tot_size = strlen(ROOTSIM_CC) + argc + strlen(add_args) + 1;
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
	strcpy(ptr, ROOTSIM_CC);
	ptr += strlen(ROOTSIM_CC);
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
		free(cmd_line);
		fprintf(stderr, "Unable to run " ROOTSIM_CC);
		return -1;
	}
	free(cmd_line);
	return 0;
}

int main(int argc, char **argv)
{
	int s_stat, p_stat;

	s_stat = child_proc(argc, argv, add_args_serial);
	p_stat = child_proc(argc, argv, add_args_parallel);

	return s_stat + p_stat;
}

