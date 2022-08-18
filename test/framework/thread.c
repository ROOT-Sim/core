/**
 * @file test/framework/thread.c
 *
 * @brief Test thread management facilities
 *
 * This module provides generic facilities for thread and core management. In particular, helper functions to startup
 * worker threads are exposed.
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <framework/thread.h>

#if defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))

#include <unistd.h>

unsigned test_thread_cores_count(void)
{
	long ret = sysconf(_SC_NPROCESSORS_ONLN);
	return ret < 1 ? 1 : (unsigned)ret;
}

int test_thread_start(thr_id_t *thr_p, thr_run_fnc t_fnc, void *t_fnc_arg)
{
	return -(pthread_create(thr_p, NULL, t_fnc, t_fnc_arg) != 0);
}

int test_thread_wait(thr_id_t thr, thrd_ret_t *ret)
{
	return -(pthread_join(thr, ret) != 0);
}

#elif defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#ifndef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_NT4
#endif
#include <windows.h>

unsigned test_thread_cores_count(void)
{
	SYSTEM_INFO sys_info;
	GetSystemInfo(&sys_info);
	return sys_info.dwNumberOfProcessors;
}

int test_thread_start(thr_id_t *thr_p, thr_run_fnc t_fnc, void *t_fnc_arg)
{
	*thr_p = CreateThread(NULL, 0, t_fnc, t_fnc_arg, 0, NULL);
	return -(*thr_p == NULL);
}

int test_thread_wait(thr_id_t thr, thrd_ret_t *ret)
{
	if(WaitForSingleObject(thr, INFINITE) == WAIT_FAILED)
		return -1;

	if(ret)
		return -(GetExitCodeThread(thr, ret) == 0);

	return 0;
}

#endif
