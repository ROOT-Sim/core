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

#include <memory.h>

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

char *args_ckpt[] = {
	"init_test",
	"--lp",
	"326",
	"--ckpt-interval",
	"95",
	NULL
};

#define TEST_INIT(args_arr, lps, threads) 				\
__extension__({								\
	init_args_parse(sizeof(args_arr) / sizeof(*(args_arr)) - 1,	\
		(args_arr));						\
	if (n_lps != lps || n_threads != threads || 			\
		memcmp(&cmp_conf, &global_config, sizeof(cmp_conf)))	\
		abort();						\
	memcpy(&cmp_conf, &default_conf, sizeof(cmp_conf));		\
})

static const struct simulation_configuration default_conf = {
	.is_serial = false,
	.core_binding = true,
	.gvt_period = 250000,
	.verbosity = 0,
	.prng_seed = 0,
	.termination_time = SIMTIME_MAX,
	.ckpt_interval = 0
};
static struct simulation_configuration cmp_conf;

int main(int argc, char **argv)
{
	(void) argc;
	(void) argv;
	memcpy(&cmp_conf, &default_conf, sizeof(cmp_conf));

	mocked_threads = 81;
	TEST_INIT(args_lp, 64, 81);

	mocked_threads = 1024;
	TEST_INIT(args_wt, 800, 400);

	cmp_conf.is_serial = true;
	TEST_INIT(args_serial, 128, 1);

	cmp_conf.core_binding = false;
	TEST_INIT(args_no_bind, 256, 1024);

	cmp_conf.gvt_period = 500000;
	TEST_INIT(args_gvt, 25, 1024);

	cmp_conf.termination_time = 1437.23;
	TEST_INIT(args_termination, 16, 1024);

	cmp_conf.prng_seed = 23491;
	TEST_INIT(args_seed, 16, 1024);

	cmp_conf.ckpt_interval = 95;
	TEST_INIT(args_ckpt, 326, 1024);

	TEST_INIT(args_lp, 64, 1024);
	return 0;
}

const struct test_config test_config = {0};
