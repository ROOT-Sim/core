/**
 * @file test/framework/thread.h
 *
* @brief Test thread management facilities
 *
 * This module provides generic facilities for thread and core management. In particular, helper functions to startup
 * worker threads are exposed.and a function to synchronize multiple threads on a software barrier.
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

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
