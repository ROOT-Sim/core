/**
* @file test/framework/mock.c
*
* @brief Mocking module
*
* This module allows to mock various parts of the core for testing purposes
*
* SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
* SPDX-License-Identifier: GPL-3.0-only
*/
#include <mock.h>

struct lp_mock {
	struct lp_ctx lp;
};

static _Thread_local struct lp_mock lp_mock;

struct lp_ctx *test_lp_mock_get(void)
{
	return &lp_mock.lp;
}
