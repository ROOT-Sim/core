/**
* @file test/framework/lp.c
*
* @brief LP mocking module
*
* This module allows to mock various parts of an LP for testing purposes
*
* SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
* SPDX-License-Identifier: GPL-3.0-only
*/
#include <stdbool.h>
#include <lp/lp.h>
#include <datatypes/list.h>

static bool initialiazed = false;

struct lp_mock_t {
	struct lp_ctx *ctx;
	struct lp_mock_t *next;
	struct lp_mock_t *prev;
};

static list(struct lp_mock_t) lp_mocks;

static void lp_exit(void)
{
	struct lp_mock_t *mock;
	while ((mock = list_head(lp_mocks)) != NULL) {
		list_delete_by_content(lp_mocks, mock);
		free(mock->ctx);
		free(mock);
	}
}

struct lp_ctx *mock_lp()
{
	struct lp_ctx *lp = malloc(sizeof(struct lp_ctx));
	memset(lp, 0, sizeof(struct lp_ctx));
	struct lib_ctx *ctx = malloc(sizeof(struct lib_ctx));
	memset(ctx, 0, sizeof(struct lib_ctx));
	lp->lib_ctx = ctx;
	ctx->rng_s[0] = 7319936632422683443ULL;
	ctx->rng_s[1] = 2268344373199366324ULL;
	ctx->rng_s[2] = 3443862242366399137ULL;
	ctx->rng_s[3] = 2366399137344386224ULL;

	if(!initialiazed) {
		lp_mocks = new_list(struct lp_mock_t);
		atexit(lp_exit);
		initialiazed = true;
	}

	struct lp_mock_t *mock = malloc(sizeof(struct lp_mock_t));
	mock->ctx = lp;
	list_insert_head(lp_mocks, mock);

	return lp;
}
