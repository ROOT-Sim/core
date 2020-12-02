/**
* @file core/init.c
*
* @brief Initialization routines
*
* This module implements the simulator initialization routines
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
#include <core/init.h>

#include <arch/arch.h>
#include <core/arg_parse.h>
#include <core/core.h>
#include <lib/lib.h>

#include <inttypes.h>
#include <limits.h>
#include <memory.h>
#include <stdlib.h>

#ifdef __POSIX
#include <unistd.h>
#define can_colorize() isatty(STDERR_FILENO)
#endif

#ifdef __WINDOWS
#include <stdio.h>
#include <io.h>
#define can_colorize() (_fileno(stderr) > 0 ? _isatty(_fileno(stderr)) : false)
#endif

simulation_configuration global_config;

/// This is the list of mnemonics for arguments
enum _opt_codes {
	OPT_NPRC,
	OPT_LOG,
	OPT_CLOG,
	OPT_SIMT,
	OPT_GVT,
	OPT_NP,
	OPT_BIND,
	OPT_SERIAL,
	OPT_LAST
};

static struct ap_option ap_options[] = {
	{"lp", 		OPT_NPRC, "VALUE", "Total number of Logical Processes being launched at simulation startup"},
	{"log-level", 	OPT_LOG,  "TYPE",  "Logging level"},
	{"time", 	OPT_SIMT, "VALUE", "Logical time at which the simulation will be considered completed"},
	{"gvt-period", 	OPT_GVT,  "VALUE", "Time between two GVT reductions (in milliseconds)"},
	{"serial", 	OPT_SERIAL, NULL,  "Runs a simulation with the serial runtime"},
	{"wt",		OPT_NP,   "VALUE", "Number of total cores being used by the simulation"},
	{"no-bind",	OPT_BIND, NULL,    "Disables thread to core binding"},
	{0}
};

static void print_config(void)
{
	// TODO
}

#define malformed_option_failure() 					\
__extension__({								\
	size_t __i;							\
	for (__i = 0; ap_options[__i].key != key; ++__i);		\
	arg_parse_error("invalid value \"%s\" in the %s option.", arg, 	\
			ap_options[__i].name);				\
})

#define parse_ullong_limits(low, high)					\
__extension__({								\
	unsigned long long int __value;					\
	char *__endptr;							\
	__value = strtoull(arg, &__endptr, 10);				\
	if (*arg == '\0' || *__endptr != '\0' ||			\
		__value < low || __value > high) {			\
		malformed_option_failure();				\
	}								\
	__value;							\
})

#define parse_ldouble_limits(low, high)					\
__extension__({								\
	long double __value;						\
	char *__endptr;							\
	__value = strtold(arg, &__endptr);				\
	if (*arg == '\0' || *__endptr != '\0' ||			\
		__value < low || __value > high) {			\
		malformed_option_failure();				\
	}								\
	__value;							\
})

static void parse_opt (int key, const char *arg)
{
	switch (key) {

	case OPT_NPRC:
		n_lps = parse_ullong_limits(1, UINT_MAX);
		break;

	case OPT_LOG:
		// TODO
		break;

	case OPT_SIMT:
		global_config.termination_time = parse_ldouble_limits(0,
			SIMTIME_MAX);
		break;

	case OPT_GVT:
		global_config.gvt_period = parse_ullong_limits(1, 10000) * 1000;
		break;

	case OPT_NP:
		n_threads = parse_ullong_limits(1, UINT_MAX);
		break;

	case OPT_BIND:
		global_config.core_binding = false;
		break;

	case OPT_SERIAL:
		global_config.is_serial = true;
		break;

	case AP_KEY_INIT:
		memset(&global_config, 0, sizeof(global_config));
		global_config.core_binding = true;
		global_config.gvt_period = 200000;
		global_config.termination_time = SIMTIME_MAX;
		log_colored = can_colorize();
		// Store the predefined values, before reading any overriding one
		// TODO
		break;

	case AP_KEY_FINI:
		// if the threads count has not been supplied, the other checks
		// are superfluous: the serial runtime simply will ignore the
		// field while the parallel one will use the set count of cores
		// (spitting a warning if it will use less cores than available)
		if (n_threads == 0) {
			n_threads = arch_core_count();
		} else if (global_config.is_serial) {
			arg_parse_error("requested a serial simulation with %u threads",
					n_threads);
		} else if (n_threads > n_lps) {
			arg_parse_error("requested a simulation with %u threads and %"PRIu64" LPs",
					n_threads, n_lps);
		} else if(n_threads > arch_core_count()) {
			arg_parse_error("demanding %u cores, which are more than available (%u)",
					n_threads, arch_core_count());
		}

		if(n_lps == 0)
			arg_parse_error("number of LPs was not provided \"--lp\"");

		log_logo_print();
		print_config();
	}
}

#undef parse_ullong_limits
#undef handle_string_option
#undef malformed_option_failure

__attribute__((weak)) struct ap_option model_options[] = {0};
__attribute__((weak)) void model_parse(int key, const char *arg){(void) key; (void) arg;}

struct ap_section ap_sects[] = {
		{NULL, ap_options, parse_opt},
		{"Model specific options", model_options, model_parse},
		{0}
};

struct ap_settings ap_sets = {
		"ROOT-Sim",	// TODO properly fill these fields
		"Proper version string",
		"piccione@diag.uniroma1.it",
		ap_sects
};

void init_args_parse(int argc, char **argv)
{
	(void) argc;
	arg_parse_run(&ap_sets, argv);
}
