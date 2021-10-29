/**
 * @file test/core/init_test.c
 *
 * @brief Test: general initialization and argument handling
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <test.h>

#include <core/arg_parse.h>
#include <core/init.h>

bool io_terminal_can_colorize(void)
{
	return false;
}

static unsigned mocked_threads;

unsigned thread_cores_count(void) {
	return mocked_threads;
}

bool log_colored;

char *args_lp[] = {
	"init_test",
	"--lp",
	"64",
	NULL
};

char *args_wt[] = {
	"init_test",
	"--wt",
	"400",
	"--lp",
	"800",
	NULL
};

char *args_serial[] = {
	"init_test",
	"--serial",
	"--lp",
	"128",
	NULL
};

char *args_no_bind[] = {
	"init_test",
	"--lp",
	"256",
	"--no-bind",
	NULL
};

char *args_gvt[] = {
	"init_test",
	"--lp",
	"25",
	"--gvt-period",
	"500",
	NULL
};

char *args_termination[] = {
	"init_test",
	"--lp",
	"16",
	"--time",
	"1437.23",
	NULL
};

char *args_seed[] = {
	"init_test",
	"--lp",
	"16",
	"--seed",
	"23491",
	NULL
};

#define TEST_INIT(args_arr, cond) 					\
__extension__({								\
	init_args_parse(sizeof(args_arr) / sizeof(*(args_arr)) - 1,	\
		(args_arr));						\
	if (!(cond)) return -1;						\
})

int main(int argc, char **argv)
{
	(void) argc;
	(void) argv;

	mocked_threads = 81;

	TEST_INIT(args_lp,
		  n_lps == 64 &&
		  n_threads == 81 &&
		  !global_config.is_serial &&
		  global_config.core_binding &&
		  global_config.gvt_period == 200000 &&
		  global_config.prng_seed == 0 &&
		  global_config.termination_time == SIMTIME_MAX);

	mocked_threads = 1024;

	TEST_INIT(args_lp,
		  n_lps == 64 &&
		  n_threads == 1024 &&
		  !global_config.is_serial &&
		  global_config.core_binding &&
		  global_config.gvt_period == 200000 &&
		  global_config.prng_seed == 0 &&
		  global_config.termination_time == SIMTIME_MAX);

	TEST_INIT(args_wt,
		  n_lps == 800 &&
		  n_threads == 400 &&
		  !global_config.is_serial &&
		  global_config.core_binding &&
		  global_config.gvt_period == 200000 &&
		  global_config.prng_seed == 0 &&
		  global_config.termination_time == SIMTIME_MAX);

	TEST_INIT(args_serial,
		  n_lps == 128 &&
		  n_threads == 1 &&
		  global_config.is_serial &&
		  global_config.core_binding &&
		  global_config.gvt_period == 200000 &&
		  global_config.prng_seed == 0 &&
		  global_config.termination_time == SIMTIME_MAX);

	TEST_INIT(args_no_bind,
		  n_lps == 256 &&
		  n_threads == 1024 &&
		  !global_config.is_serial &&
		  !global_config.core_binding &&
		  global_config.gvt_period == 200000 &&
		  global_config.prng_seed == 0 &&
		  global_config.termination_time == SIMTIME_MAX);

	TEST_INIT(args_gvt,
		  n_lps == 25 &&
		  n_threads == 1024 &&
		  !global_config.is_serial &&
		  global_config.core_binding &&
		  global_config.gvt_period == 500000 &&
		  global_config.prng_seed == 0 &&
		  global_config.termination_time == SIMTIME_MAX);

	TEST_INIT(args_termination,
		  n_lps == 16 &&
		  n_threads == 1024 &&
		  !global_config.is_serial &&
		  global_config.core_binding &&
		  global_config.gvt_period == 200000 &&
		  global_config.prng_seed == 0 &&
		  global_config.termination_time == 1437.23);

	TEST_INIT(args_seed,
		  n_lps == 16 &&
		  n_threads == 1024 &&
		  !global_config.is_serial &&
		  global_config.core_binding &&
		  global_config.gvt_period == 200000 &&
		  global_config.prng_seed == 23491 &&
		  global_config.termination_time == SIMTIME_MAX);

	return 0;
}

const struct test_config test_config = {0};
