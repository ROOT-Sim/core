/**
* @file test/test.h
*
* @brief Custom minimalistic testing framework
*
* SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
* SPDX-License-Identifier: GPL-3.0-only
*/
#pragma once

#include <stdint.h>

#ifndef _unused
#define _unused __attribute__ ((unused))
#endif

typedef int (*test_fn)(void *);

extern uint64_t test_random_range(uint64_t n);
extern uint64_t test_random_u(void);
extern double test_random_double(void);

extern __attribute__((noreturn)) void test_fail(void);
extern int test(const char *desc, test_fn test_fn, void *arg);
extern int test_xf(const char *desc, test_fn test_fn, void *arg);
extern int test_parallel(const char *desc, test_fn test_fn, void *args, unsigned thread_count);
extern unsigned test_parallel_thread_id(void);

extern void test_assert_internal(_Bool condition, const char *file_name, unsigned line_count);
#define test_assert(condition) test_assert_internal(condition, __FILE__, __LINE__)

extern struct lp_ctx *test_lp_mock_get();
