/**
* @file test/framework/mock.h
*
* @brief Mocking module
*
* This module allows to mock various parts of the core for testing purposes
*
* SPDX-FileCopyrightText: 2008-2025 HPDCS Group <rootsim@googlegroups.com>
* SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <lp/lp.h>

extern struct lp_ctx *test_lp_mock_get(void);
