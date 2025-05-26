/**
 * @file test/self/stubs.h
 *
 * @brief Test: Test core functions of the testing framework
 *
 * SPDX-FileCopyrightText: 2008-2023 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

extern int test_want_arg_null(void *arg);
extern int test_assert_arg_null(void *arg);
extern int test_fail_on_not_null(void *arg);
