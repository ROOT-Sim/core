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
};

static _Thread_local struct lp_mock lp_mock;

struct lp_ctx *test_lp_mock_get(void)
{
	return &lp_mock.lp;
}
