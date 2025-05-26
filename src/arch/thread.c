/**
 * @file arch/thread.c
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
#include <arch/thread.h>

/**
 * @fn thread_start(thr_id_t *thr_p, thr_run_fnc t_fnc, void *t_fnc_arg)
 * @brief Creates a thread
 * @param thr_p A pointer to the location where the created thread identifier
 *              will be copied
 * @param t_fnc The new thread entry point
 * @param t_fnc_arg A pointer to the argument to be passed to the new thread
 *                  entry point
 * @return 0 if successful, -1 otherwise
 */

/**
 * @fn thread_affinity_self_set(unsigned core)
 * @brief Sets a core affinity for the currently running thread
 * @param core The core id where the target thread will be pinned on
 * @return a #thread_affinity_error value
 */

/**
 * @fn thread_wait(thr_id_t thr, thrd_ret_t *ret)
 * @brief Wait for specified thread to complete execution
 * @param thr The identifier of the thread to wait for
 * @param ret A pointer to the location where the return value will be copied,
 *            or alternatively NULL
 * @return 0 if successful, -1 otherwise
 */

/**
 * @fn thread_cores_count(void)
 * @brief Computes the count of available cores on the machine
 * @return the count of the processing cores available on the machine
 */

#ifdef __POSIX
#include <sched.h>

#ifdef __MACOS
#include <mach/thread_act.h>
#include <unistd.h>

enum thread_affinity_error thread_affinity_self_set(unsigned core)
{
	thread_affinity_policy_data_t policy = {core + 1};
	pthread_t self = pthread_self();
	thread_port_t mach_thread = pthread_mach_thread_np(self);

	kern_return_t ret = thread_policy_set(mach_thread, THREAD_AFFINITY_POLICY, (thread_policy_t)&policy,
	    THREAD_AFFINITY_POLICY_COUNT);

	switch(ret) {
		case KERN_SUCCESS:
			return THREAD_AFFINITY_SUCCESS;
		case KERN_NOT_SUPPORTED:
			return THREAD_AFFINITY_ERROR_NOT_SUPPORTED;
		default:
			return THREAD_AFFINITY_ERROR_RUNTIME;
	}
}

unsigned thread_cores_count(void)
{
	long ret = sysconf(_SC_NPROCESSORS_ONLN);
	return ret < 1 ? 1 : (unsigned)ret;
}

#else

#include <errno.h>

// FIXME: it's quadratic in the number of cores when used core times
enum thread_affinity_error thread_affinity_self_set(unsigned core)
{
	cpu_set_t cpuset;
	sched_getaffinity(0, sizeof(cpuset), &cpuset);

	for(unsigned i = 0; i < CPU_SETSIZE; ++i) {
		if(!CPU_ISSET(i, &cpuset))
			continue;

		if(core == 0) {
			CPU_ZERO(&cpuset);
			CPU_SET(i, &cpuset);

			const pthread_t self = pthread_self();
			switch(pthread_setaffinity_np(self, sizeof(cpuset), &cpuset)) {
				case 0:
					return THREAD_AFFINITY_SUCCESS;
				case EINVAL:
					return THREAD_AFFINITY_ERROR_NOT_SUPPORTED;
				default:
					return THREAD_AFFINITY_ERROR_RUNTIME;
			}
		}
		--core;
	}
	return THREAD_AFFINITY_ERROR_RUNTIME;
}

unsigned thread_cores_count(void)
{
	cpu_set_t cpuset;
	sched_getaffinity(0, sizeof(cpuset), &cpuset);

	unsigned cores = 0;
	for(long i = 0; i < CPU_SETSIZE; ++i)
		cores += CPU_ISSET(i, &cpuset) != 0;

	return cores < 1 ? 1 : cores;
}

#endif

int thread_start(thr_id_t *thr_p, const thr_run_fnc t_fnc, void *t_fnc_arg)
{
	return -(pthread_create(thr_p, NULL, t_fnc, t_fnc_arg) != 0);
}

int thread_wait(const thr_id_t thr, thrd_ret_t *ret)
{
	return -(pthread_join(thr, ret) != 0);
}

#endif

#ifdef __WINDOWS

#include <limits.h>

#define WIN32_LEAN_AND_MEAN
#ifndef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_NT4
#endif
#include <windows.h>

unsigned thread_cores_count(void)
{
	SYSTEM_INFO sys_info;
	GetSystemInfo(&sys_info);
	return sys_info.dwNumberOfProcessors;
}

int thread_start(thr_id_t *thr_p, thr_run_fnc t_fnc, void *t_fnc_arg)
{
	*thr_p = CreateThread(NULL, 0, t_fnc, t_fnc_arg, 0, NULL);
	return -(*thr_p == NULL);
}

int thread_affinity_self_set(unsigned core)
{
	DWORD_PTR proc_mask, sys_mask;
	HANDLE self_process = GetCurrentProcess();
	if(!GetProcessAffinityMask(self_process, &proc_mask, &sys_mask))
		return THREAD_AFFINITY_ERROR_RUNTIME;

	for(DWORD i = 0; i < sizeof(DWORD_PTR) * CHAR_BIT; ++i) {
		DWORD_PTR target_mask = (DWORD_PTR)1U << i;
		if(!(proc_mask & target_mask))
			continue;

		if(core == 0) {
			HANDLE self_thread = GetCurrentThread();
			DWORD_PTR res = SetThreadAffinityMask(self_thread, target_mask);
			if(res)
				return THREAD_AFFINITY_SUCCESS;

			if(GetLastError() == ERROR_ACCESS_DENIED)
				return THREAD_AFFINITY_ERROR_NOT_SUPPORTED;

			return THREAD_AFFINITY_ERROR_RUNTIME;
		}
		--core;
	}

	return THREAD_AFFINITY_ERROR_RUNTIME;
}

int thread_wait(thr_id_t thr, thrd_ret_t *ret)
{
	if(WaitForSingleObject(thr, INFINITE) == WAIT_FAILED)
		return -1;

	if(ret)
		return -(GetExitCodeThread(thr, ret) == 0);

	return 0;
}

#endif
