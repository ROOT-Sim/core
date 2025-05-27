/**
 * @file test/thread.c
 *
 * @brief Test thread management facilities
 *
 * This module provides generic facilities for thread and core management. In particular, helper functions to startup
 * worker threads are exposed.
 *
 * SPDX-FileCopyrightText: 2008-2023 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <test.h>

#if defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))

#include <unistd.h>

/**
 * @brief Retrieves the number of available CPU cores.
 *
 * This function uses the `sysconf` system call to determine the number of online processors.
 * If the result is less than 1, it defaults to 1.
 *
 * @return The number of available CPU cores.
 */
unsigned test_thread_cores_count(void)
{
	long ret = sysconf(_SC_NPROCESSORS_ONLN);
	return ret < 1 ? 1 : (unsigned)ret;
}


/**
 * @brief Starts a new thread.
 *
 * This function creates a new thread using the `pthread_create` function.
 *
 * @param thr_p A pointer to the thread identifier to be initialized.
 * @param t_fnc The function to be executed by the new thread.
 * @param t_fnc_arg The argument to be passed to the thread function.
 * @return 0 on success, or -1 on failure.
 */
int test_thread_start(thr_id_t *thr_p, thr_run_fnc t_fnc, void *t_fnc_arg)
{
	return -(pthread_create(thr_p, NULL, t_fnc, t_fnc_arg) != 0);
}

/**
 * @brief Waits for a thread to finish execution.
 *
 * This function blocks until the specified thread terminates.
 *
 * @param thr The thread identifier of the thread to wait for.
 * @param ret A pointer to store the return value of the thread function.
 * @return 0 on success, or -1 on failure.
 */
int test_thread_wait(thr_id_t thr, thrd_ret_t *ret)
{
	return -(pthread_join(thr, ret) != 0);
}


/**
 * @brief Suspends the execution of the current thread for a specified duration.
 *
 * This function uses `nanosleep` to pause the current thread for the given number of milliseconds.
 *
 * @param milliseconds The duration to sleep, in milliseconds.
 */
void test_thread_sleep(unsigned milliseconds)
{
	struct timespec ts;
	ts.tv_sec = milliseconds / 1000;
	ts.tv_nsec = (milliseconds % 1000) * 1000000;
	nanosleep(&ts, NULL);
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

void test_thread_sleep(unsigned milliseconds)
{
	Sleep(milliseconds);
}

#endif
