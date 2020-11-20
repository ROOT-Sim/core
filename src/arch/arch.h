/**
 * @file arch/arch.h
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

#include <stdbool.h>

#include <arch/platform.h>

#if defined(__POSIX)
#include <pthread.h>

#define ARCH_CALL_CONV
typedef void * arch_thr_ret_t;
typedef pthread_t arch_thr_t;

#define ARCH_THR_RET_FAILURE ((void *) 1)
#define ARCH_THR_RET_SUCCESS ((void *) 0)

#elif defined(__WINDOWS)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN

#define ARCH_CALL_CONV WINAPI
typedef DWORD arch_thr_ret_t;
typedef HANDLE arch_thr_t;

#define ARCH_THR_RET_FAILURE (1)
#define ARCH_THR_RET_SUCCESS (0)

#endif

typedef arch_thr_ret_t ARCH_CALL_CONV (*arch_thr_fnc)(void *);

extern int arch_thread_create(arch_thr_t *thr_p, arch_thr_fnc t_fnc, void *t_fnc_arg);
extern int arch_thread_affinity_set(arch_thr_t thr, unsigned core);
extern int arch_thread_wait(arch_thr_t thr, arch_thr_ret_t *ret);
extern unsigned arch_core_count(void);
