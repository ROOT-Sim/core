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
 * @copyright
 * Copyright (C) 2008-2020 HPDCS Group
 * https://rootsim.github.io/core
 *
 * This file is part of ROOT-Sim (ROme OpTimistic Simulator).
 *
 * ROOT-Sim is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; only version 3 of the License applies.
 *
 * ROOT-Sim is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ROOT-Sim; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#pragma once

#include <arch/platform.h>

#if defined(__POSIX)
#define _GNU_SOURCE
#include <pthread.h>

#define THREAD_CALL_CONV
typedef void * thr_ret_t;
typedef pthread_t thr_id_t;

#define THREAD_RET_FAILURE ((void *) 1)
#define THREAD_RET_SUCCESS ((void *) 0)

#elif defined(__WINDOWS)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN

#define THREAD_CALL_CONV WINAPI
typedef DWORD arch_thr_ret_t;
typedef HANDLE arch_thr_t;

#define THREAD_RET_FAILURE (1)
#define THREAD_RET_SUCCESS (0)

#endif

/// The function type of a new thread entry point
typedef thr_ret_t THREAD_CALL_CONV (*thr_run_fnc)(void *);

extern int thread_create(thr_id_t *thr_p, thr_run_fnc t_fnc, void *t_fnc_arg);
extern int thread_affinity_set(thr_id_t thr, unsigned core);
extern int thread_wait(thr_id_t thr, thr_ret_t *ret);
extern unsigned thread_cores_count(void);
