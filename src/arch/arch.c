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
#include <arch/arch.h>

#ifdef __POSIX
#include <sched.h>
#include <signal.h>
#include <unistd.h>

#ifdef __MACOS
#define _DARWIN_C_SOURCE
#include <stdint.h>
#include <mach/mach_types.h>
#include <mach/thread_act.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/sysctl.h>


typedef struct cpu_set {
	uint32_t count;
} cpu_set_t;

static inline void CPU_ZERO(cpu_set_t *cs)
{
	cs->count = 0;
}

static inline void CPU_SET(int num, cpu_set_t *cs) {
	cs->count |= (1 << num);
}

static inline int CPU_ISSET(int num, cpu_set_t *cs)
{
	return (cs->count & (1 << num));
}

static inline int pthread_setaffinity_np(pthread_t thread, size_t cpu_size, cpu_set_t *cpu_set)
{
	thread_port_t mach_thread;
	unsigned core = 0;

	for(core = 0; core < 8 * cpu_size; core++) {
		if(CPU_ISSET(core, cpu_set))
			break;
	}

	thread_affinity_policy_data_t policy = {core};
	mach_thread = pthread_mach_thread_np(thread);
	thread_policy_set(mach_thread, THREAD_AFFINITY_POLICY, (thread_policy_t) &policy, 1);
	return 0;
}
#endif

/**
 * @brief Computes the count of available cores on the machine
 * @return the count of the processing cores available on the machine
 */
unsigned arch_core_count(void)
{
#ifdef __MACOS
	int32_t ret = 0;
	size_t  len = sizeof(ret);
	if(sysctlbyname("machdep.cpu.core_count", &ret, &len, 0, 0)) {
		ret = 0;
	}
#else
	long int ret = sysconf(_SC_NPROCESSORS_ONLN);
#endif
	return ret < 1 ? 1 : (unsigned)ret;
}

/**
 * @brief Creates a thread
 *
 * @param thr_p A pointer to the location where the created thread identifier
 *              will be copied
 * @param t_fnc The new thread entry point
 * @param t_fnc_arg A pointer to the argument to be passed to the new thread
 *                  entry point
 * @return 0 if successful, -1 otherwise
 */
int arch_thread_create(arch_thr_t *thr_p, arch_thr_fnc t_fnc, void *t_fnc_arg)
{
	return -(pthread_create(thr_p, NULL, t_fnc, t_fnc_arg) != 0);
}

/**
 * @brief Sets a core affinity for a thread
 *
 * @param thr The identifier of the thread targeted for the affinity change
 * @param core The core id where the target thread will be pinned on
 * @return 0 if successful, -1 otherwise
 */
int arch_thread_affinity_set(arch_thr_t thr, unsigned core)
{
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(core, &cpuset);
	return -(pthread_setaffinity_np(thr, sizeof(cpuset), &cpuset) != 0);
}

/**
 * @brief Wait for specified thread to complete execution
 *
 * @param thr The identifier of the thread to wait for
 * @param ret A pointer to the location where the return value will be copied,
 *            or alternatively NULL
 * @return 0 if successful, -1 otherwise
 */
int arch_thread_wait(arch_thr_t thr, arch_thr_ret_t *ret)
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

unsigned arch_core_count(void)
{
	SYSTEM_INFO sys_info;
	GetSystemInfo(&sys_info);
	return sys_info.dwNumberOfProcessors;
}

int arch_thread_create(arch_thr_t *thr_p, arch_thr_fnc t_fnc, void *arg)
{
	*thr_p = CreateThread(NULL, 0, t_fnc, arg, 0, NULL);
	return -(*thr_p == NULL);
}

int arch_thread_affinity_set(arch_thr_t thr, unsigned core)
{
	return -(SetThreadAffinityMask(thr, 1 << core) != 0);
}

int arch_thread_wait(arch_thr_t thr, arch_thr_ret_t *ret)
{
	if (WaitForSingleObject(thr, INFINITE) == WAIT_FAILED)
		return -1;

	if (ret)
		return -(GetExitCodeThread(thr, ret) == 0);

	return 0;
}

#endif
