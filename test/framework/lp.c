/**
* @file test/framework/lp.c
*
* @brief LP mocking module
*
* This module allows to mock various parts of an LP for testing purposes
*
* SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
* SPDX-License-Identifier: GPL-3.0-only
*/
#include <lp/lp.h>

struct lp_mock {
	struct lp_ctx lp;
	struct rng_ctx rng_ctx;
};

static _Thread_local struct lp_mock lp_mock;

struct lp_ctx *test_lp_mock_get(void)
{
	lp_mock.lp.rng_ctx = &lp_mock.rng_ctx;
	lp_mock.rng_ctx.state[0] = 7319936632422683443ULL;
	lp_mock.rng_ctx.state[1] = 2268344373199366324ULL;
	lp_mock.rng_ctx.state[2] = 3443862242366399137ULL;
	lp_mock.rng_ctx.state[3] = 2366399137344386224ULL;

	return &lp_mock.lp;
}
