/**
 * @file arch/timer.h
 *
 * @brief Timers
 *
 * This header defines the timers which the simulator uses to monitor its
 * internal behaviour
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <arch/platform.h>

#include <stdint.h>
#include <time.h>

/** The type used to store results of timer related calls */
typedef uint_fast64_t timer_uint;

#ifdef __POSIX
#include <sys/time.h>

inline timer_uint timer_new(void)
{
	struct timeval tmptv;
	gettimeofday(&tmptv, NULL);
	return (timer_uint) tmptv.tv_sec * 1000000U + tmptv.tv_usec;
}

inline timer_uint timer_value(timer_uint start)
{
	return timer_new() - start;
}

#endif

#ifdef __WINDOWS
#include <windows.h>

extern timer_uint timer_perf_freq;

inline timer_uint timer_new(void)
{
	LARGE_INTEGER start_time;
	QueryPerformanceCounter(&start_time);
	return (timer_uint)start_time.QuadPart;
}


inline timer_uint timer_value(timer_uint start)
{
	if (unlikely(timer_perf_freq == 0)) {
		LARGE_INTEGER perf;
		QueryPerformanceFrequency(&perf);
		timer_perf_freq = perf.QuadPart;
	}
	return (timer_new() - start) * 1000000U / timer_perf_freq;
}

#endif

