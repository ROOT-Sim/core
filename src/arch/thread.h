/**
 * @file arch/thread.h
 *
 * @brief Generic architecture management facilities
 *
 * This module provides generic facilities for thread and core management.
 * In particular, helper functions to startup worker threads are exposed,
 * and a function to synchronize multiple threads on a software barrier.
 *
 * The software barrier also offers a leader election facility, so that
 * once all threads are synchronized on the barrier, the function returns
 * true to only one of them.
 *
 * SPDX-FileCopyrightText: 2008-2025 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <arch/platform.h>

#if defined(__POSIX)
#include <pthread.h>

#define THREAD_CALL_CONV
typedef void *thrd_ret_t;
typedef pthread_t thr_id_t;

#define THREAD_RET_FAILURE ((void *)1)
#define THREAD_RET_SUCCESS ((void *)0)

#elif defined(__WINDOWS)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN

#define THREAD_CALL_CONV WINAPI
typedef DWORD thrd_ret_t;
typedef HANDLE thr_id_t;

#define THREAD_RET_FAILURE (1)
#define THREAD_RET_SUCCESS (0)
#endif

/// The function type of a new thread entry point
typedef thrd_ret_t(THREAD_CALL_CONV *thr_run_fnc)(void *);

/// The return values of thread_affinity_set()
enum thread_affinity_error {
	/// The thread affinity was set successfully
	THREAD_AFFINITY_SUCCESS = 0,
	/// A runtime error occurred during the thread affinity set operation
	THREAD_AFFINITY_ERROR_RUNTIME = -1,
	/// The requested affinity is not supported by the OS
	THREAD_AFFINITY_ERROR_NOT_SUPPORTED = -2,
};

extern int thread_start(thr_id_t *thr_p, thr_run_fnc t_fnc, void *t_fnc_arg);
extern enum thread_affinity_error thread_affinity_self_set(unsigned core);
extern int thread_wait(thr_id_t thr, thrd_ret_t *ret);
extern unsigned thread_cores_count(void);
