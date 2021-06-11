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
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
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
 * @fn thread_affinity_set(thr_id_t thr, unsigned core)
 * @brief Sets a core affinity for a thread
 * @param thr The identifier of the thread targeted for the affinity change
 * @param core The core id where the target thread will be pinned on
 * @return 0 if successful, -1 otherwise
 */

/**
 * @fn thread_wait(thr_id_t thr, thr_ret_t *ret)
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
#include <signal.h>
#include <unistd.h>

#ifdef __MACOS
#include <mach/thread_act.h>

int thread_affinity_set(thr_id_t thr, unsigned core)
{
	thread_affinity_policy_data_t policy = {core};
	thread_port_t mach_thread = pthread_mach_thread_np(thr);
	kern_return_t ret = thread_policy_set(mach_thread, THREAD_AFFINITY_POLICY,
					      (thread_policy_t) &policy, 1);
	return -(ret != KERN_SUCCESS);
}

#else

int thread_affinity_set(thr_id_t thr, unsigned core)
{
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(core, &cpuset);
	return -(pthread_setaffinity_np(thr, sizeof(cpuset), &cpuset) != 0);
}

#endif

unsigned thread_cores_count(void)
{
	long ret = sysconf(_SC_NPROCESSORS_ONLN);
	return ret < 1 ? 1 : (unsigned)ret;
}

int thread_start(thr_id_t *thr_p, thr_run_fnc t_fnc, void *t_fnc_arg)
{
	return -(pthread_create(thr_p, NULL, t_fnc, t_fnc_arg) != 0);
}

int thread_wait(thr_id_t thr, thr_ret_t *ret)
{
	return -(pthread_join(thr, ret) != 0);
}

#endif

#ifdef __WINDOWS
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

int thread_affinity_set(thr_id_t thr, unsigned core)
{
	return -(SetThreadAffinityMask(thr, 1 << core) == 0);
}

int thread_wait(thr_id_t thr, thr_ret_t *ret)
{
	if (WaitForSingleObject(thr, INFINITE) == WAIT_FAILED)
		return -1;

	if (ret)
		return -(GetExitCodeThread(thr, ret) == 0);

	return 0;
}

#endif
