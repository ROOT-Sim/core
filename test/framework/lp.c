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
#include <datatypes/list.h>
#include <lp/lp.h>

#include <stdbool.h>

struct lp_mock_t {
	struct lp_ctx lp;
	struct lib_ctx lp_lib;
	struct lp_mock_t *next;
	struct lp_mock_t *prev;
};

static list(struct lp_mock_t) lp_mocks;

static void lp_exit(void)
{
	struct lp_mock_t *mock;
	while ((mock = list_head(lp_mocks)) != NULL) {
		list_detach_by_content(lp_mocks, mock);
		free(mock);
	}
}

struct lp_ctx *mock_lp()
{
	struct lp_mock_t *mock = malloc(sizeof(*mock));
	memset(mock, 0, sizeof(*mock));

	struct lp_ctx *lp = &mock->lp;
	struct lib_ctx *ctx = &mock->lp_lib;

	lp->lib_ctx = ctx;
	ctx->rng_s[0] = 7319936632422683443ULL;
	ctx->rng_s[1] = 2268344373199366324ULL;
	ctx->rng_s[2] = 3443862242366399137ULL;
	ctx->rng_s[3] = 2366399137344386224ULL;

	static bool initialized = false;
	if(!initialized) {
		lp_mocks = new_list(struct lp_mock_t);
		atexit(lp_exit);
		initialized = true;
	}

	list_insert_head(lp_mocks, mock);

	return lp;
}
