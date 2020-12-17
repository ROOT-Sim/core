/**
* @file core/timer.h
*
* @brief Timers
*
* This header defines the timers which the simulator uses to monitor its
* internal behaviour
*
* @copyright
* Copyright (C) 2008-2020 HPDCS Group
* https://hpdcs.github.io
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

#include <stdint.h>
#include <time.h>

/** The type used to store results of timer related calls */
typedef uint_fast64_t timer_uint;

/**
 * @fn timer_new
 * @brief Gets a new starting point for an time interval measure
 * @returns a timer_uint value, a not meaningful value by itself
 *
 * The returned value can be used in the timer_value() macro to obtain a time
 * interval with microsecond resolution
 */

/**
 * @fn timer_value
 * @brief Computes a time interval measure using a previous timer_uint value
 * @param timer a timer_uint value obtained from a previous timer_new() call
 * @return a timer_uint value, the count of microseconds of the time interval
 */

#ifdef __POSIX
#include <sys/time.h>

inline timer_uint timer_new(void)
{
	struct timeval tmptv;
	gettimeofday(&tmptv, NULL);
	return (timer_uint) tmptv.tv_sec * 1000000 + tmptv.tv_usec;
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
	LARGE_INTEGER __start_time;
	QueryPerformanceCounter(&__start_time);
	return (timer_uint)__start_time.QuadPart;
}


inline timer_uint timer_value(timer_uint start)
{
	if(unlikely(timer_perf_freq == 0)) {
		LARGE_INTEGER __perf;
		QueryPerformanceFrequency(&__perf);
		timer_perf_freq = __perf.QuadPart;
	}
	return (timer_new() - start) / timer_perf_freq;
}

#endif

