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

#if defined(__linux__)

#define _GNU_SOURCE
#include <core/core.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <unistd.h>

static pthread_t *ptids;

static void signal_mask_set(bool ignore_sigint)
{
	sigset_t mask, old_mask;
	sigemptyset(&mask);
	if (ignore_sigint)
		sigaddset(&mask, SIGINT);
	pthread_sigmask(SIG_BLOCK, &mask, &old_mask);
}

unsigned arch_core_count(void)
{
	long int ret = sysconf(_SC_NPROCESSORS_ONLN);
	return ret < 1 ? 1 : (unsigned)ret;
}

void arch_thread_create(
	unsigned t_cnt,
	bool affinity,
	void *(*t_fnc)(void *),
	void *t_fnc_arg
){
	ptids = mm_alloc(sizeof(*ptids) * t_cnt);

	pthread_attr_t t_attr;
	pthread_attr_init(&t_attr);

	signal_mask_set(true);

	while(t_cnt--){
		cpu_set_t c_set;

		CPU_ZERO(&c_set);
		CPU_SET(t_cnt, &c_set);

		if(affinity)
			pthread_attr_setaffinity_np(
				&t_attr, sizeof(c_set), &c_set);

		pthread_create(&ptids[t_cnt], &t_attr, t_fnc, t_fnc_arg);
	}

	pthread_attr_destroy(&t_attr);

	signal_mask_set(false);
}

void arch_thread_wait(unsigned t_cnt)
{
	while(t_cnt--){
		pthread_join(ptids[t_cnt], NULL);
	}

	mm_free(ptids);
}

#elif defined(_WIN64)

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0400
#include <windows.h>

unsigned arch_core_count(void)
{
	SYSTEM_INFO sys_info;
	GetSystemInfo(&sys_info);
	return sys_info.dwNumberOfProcessors;
}

void arch_thread_init(
	unsigned thread_count,
	bool set_affinity,
	void *(*thread_fnc)(void *),
	void *thread_fnc_arg
){
	while (thread_count--) {
		HANDLE t_new = CreateThread(NULL, 0, thread_fnc, thread_fnc_arg,
			0, NULL);

		if(set_affinity)
			SetThreadAffinityMask(t_new, 1 << thread_count);
	}
}

#else /* OS_LINUX || OS_WINDOWS */
#error Unsupported operating system
#endif
