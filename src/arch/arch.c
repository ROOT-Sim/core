/**
 * @file arch/arch.c
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
#define _GNU_SOURCE
#include <stdbool.h>
#include <assert.h>

#include <arch/arch.h>
#include <core/core.h>

#ifdef __POSIX
#include <signal.h>
#include <unistd.h>
#include <sched.h>

/**
* @brief Set the affinity of invoking thread
*
* If called, the caller thread returns stuck on the specified core
*
* @param core The core to run on
*/
static void set_my_affinity(unsigned core) {
	cpu_set_t c_set;
	CPU_ZERO(&c_set);
	CPU_SET(core, &c_set);
	sched_setaffinity(0, sizeof(c_set), &c_set);
}

static unsigned arch_core_count(void)
{
	long int ret = sysconf(_SC_NPROCESSORS_ONLN);
	return ret < 1 ? 1 : (unsigned)ret;
}

/**
* @brief Ignore signals
*
* Ignore signals sent to the simulation core.
*
* @param ignore_sigint If set to true, SIGINT is ignored
*/
static void signal_mask_set(bool ignore_sigint)
{
	sigset_t mask, old_mask;
	sigemptyset(&mask);
	if (ignore_sigint)
		sigaddset(&mask, SIGINT);
	pthread_sigmask(SIG_BLOCK, &mask, &old_mask);
}
#endif

#ifdef __WINDOWS
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT _WIN32_WINNT_NT4
#include <windows.h>

static inline void set_my_affinity(unsigned core)
{
	SetThreadAffinityMask(GetCurrentThread(), 1 << core);
}

static inline unsigned arch_core_count(void)
{
	SYSTEM_INFO sys_info;
	GetSystemInfo(&sys_info);
	return sys_info.dwNumberOfProcessors;
}

static inline void signal_mask_set(bool ignore_sigint)
{
}
#endif

/// Thread ids of the activated worker threads
static thrd_t *thr_ids;

/**
 * This helper function is the actual entry point for every thread created
 * using the provided internal services.
 *
 * Additionally, when the created thread returns, it frees the memory used
 * to maintain the real entry point and the pointer to its arguments.
 *
 * @param arg A pointer to an internally defined structure of type
 *            struct t_params keeping the real thread's entry point,
 *            its arguments, and information about affinity.
 *
 * @return This function always returns 0
 */
static int __helper_create_thread(void *args) {
	struct t_params *params = (struct t_params *)args;
	thrd_start_t entry = params->entry;
	args = params->args;

	if(likely(params->set_affinity)) {
		set_my_affinity(params->t_cnt);
	}

	mm_free(params);

	entry(args);
	return 0;
}

/**
 * @brief span a number of threads
 *
 * This function creates n threads, all having the same entry point and
 * a pointer to arguments. The entry point is specified by the function t_fnc.
 * Thread creation relies on the __helper_create_thread() function, to properly
 * setup affinity in a C11 portable way, without relying on non-portable
 * pthread interfaces. The startup cost will be a little higher, but in the end
 * this should allow to compile on more platforms with less hussle.
 *
 * Note that the arguments passed to __helper_create_thread are malloc'd
 * here, and free'd there.
 * Additionally, note that we don't make a copy of the arguments pointed
 * by arg, so it is the responsibility of the caller to preserve them
 * during the thread's lifetime.
 * Changing passed arguments from one of the newly created threads will result
 * in all the threads seeing the change.
 *
 * @param t_cnt The number of threads which should be created
 * @param set_affinity If set to true, set threads set_affinity so as to run on
 *        different cores.
 * @param t_fnc The new threads' entry point
 * @param t_fnc_arg A pointer to an array of arguments to be passed to the
 *            new threads' entry point
 */
void arch_thread_create(unsigned t_cnt, bool set_affinity, thrd_start_t t_fnc, void *t_fnc_arg)
{
	struct t_params *params;
	// We are assumed to call this function only once
	assert(thr_ids == NULL);

	thr_ids = mm_alloc(sizeof(*thr_ids) * t_cnt);
	thr_ids[t_cnt] = 0;

	signal_mask_set(true);

	while(t_cnt--) {
		params = mm_alloc(sizeof(*params));
		params->entry = t_fnc;
		params->args = t_fnc_arg;
		params->set_affinity = set_affinity;
		params->t_cnt = t_cnt;

		thrd_create(&thr_ids[t_cnt], __helper_create_thread, params);
	}

	signal_mask_set(false);
}

/**
 * @brief Wait for processing threads to complete execution
 *
 * This function suspends execution in the main thread, until all remainin threads
 * have correctly returned.
 * This function shall be called after invoking arch_thread_create().
 */
void arch_thread_wait(void)
{
	size_t thr = 0;
	assert(thr_ids != NULL);

	while(thr_ids[thr] != 0) {
		thrd_join(thr_ids[thr++], NULL);
	}

	mm_free(thr_ids);
}

