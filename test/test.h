/**
 * @file test/test.h
 *
 * @brief Custom minimalistic testing framework
 *
 * SPDX-FileCopyrightText: 2008-2023 HPDCS Group <rootsim@googlegroups.com>
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

typedef __uint128_t test_rng_state;

extern void rng_init(test_rng_state *rng_state, test_rng_state initseq);
extern uint64_t rng_random_u(test_rng_state *rng_state);
extern double rng_random(test_rng_state *rng_state);
extern uint64_t rng_random_range(test_rng_state *rng_state, uint64_t n);
extern int rng_ks_test(uint32_t n_samples, double (*sample)(void));


#if defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
#include <pthread.h>

#define THREAD_CALL_CONV
typedef void *thrd_ret_t;
typedef pthread_t thr_id_t;

#define THREAD_RET_FAILURE ((void *)1)
#define THREAD_RET_SUCCESS ((void *)0)

#elif defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN

#define THREAD_CALL_CONV WINAPI
typedef DWORD thrd_ret_t;
typedef HANDLE thr_id_t;

#define THREAD_RET_FAILURE (1)
#define THREAD_RET_SUCCESS (0)

#else
#error Unsupported operating system
#endif

/// The function type of a new thread entry point
typedef thrd_ret_t(THREAD_CALL_CONV *thr_run_fnc)(void *);

extern int test_thread_start(thr_id_t *thr_p, thr_run_fnc t_fnc, void *t_fnc_arg);
extern int test_thread_wait(thr_id_t thr, thrd_ret_t *ret);
extern unsigned test_thread_cores_count(void);
extern void test_thread_sleep(unsigned milliseconds);
