/**
 * @file test/gvt/gvt_test.c
 *
 * @brief Test: parallel gvt algorithm
 *
 * SPDX-FileCopyrightText: 2008-2023 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <test.h>

#include <core/init.h>
#include <gvt/gvt.h>

#define N_THREADS 3

static simtime_t bound_values[N_THREADS][6] = {
	{1.1, 4.1, 6.7, 7.0, 9.8, 10.0},
	{2.0, 3.4, 6.5, 6.5, 9.6, 11.0},
	{1.2, 3.5, 6.4, 6.3, 9.7, 10.5},
};
static __thread unsigned b_i = 0;

struct simulation_configuration global_config = {
	.gvt_period = 1000
};


static int gvt_test_init(void)
{
	gvt_global_init();
	return 0;
}

static int gvt_test(void)
{
	int ret = 0;

	while(!gvt_msg_processed());
	ret -= current_gvt != 1.1;

	while(!gvt_msg_processed());
	ret -= current_gvt != 6.3;

	while(!gvt_msg_processed());
	ret -= current_gvt != 9.6;

	return ret;
}

const struct test_config test_config = {
	.threads_count = N_THREADS,
	.test_init_fnc = gvt_test_init,
	.test_fnc = gvt_test
};
