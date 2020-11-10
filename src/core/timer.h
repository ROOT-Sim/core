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

#include <time.h>
#include <sys/time.h>

/* TODO this is Linux-only, port to Windows using QueryPerformanceFrequency()
 * and QueryPerformanceCounter() */

/** The type used to store results of timer related calls */
typedef uint_fast64_t timer_uint;

/**
 * @brief Gets a new starting point for an time interval measure
 * @returns a timer_uint value, a not meaningful value by itself
 *
 * The returned value can be used in the timer_value() macro to obtain a time
 * interval with microsecond resolution
 */
#define timer_new()							\
__extension__({								\
	struct timeval __tmptv;						\
	gettimeofday(&__tmptv, NULL);					\
	(timer_uint) __tmptv.tv_sec * 1000000 + __tmptv.tv_usec;	\
})

/**
 * @brief Computes a time interval measure using a previous timer_uint value
 * @param timer a timer_uint value obtained from a previous timer_new() call
 * @returns a timer_uint value, the count of microseconds of the time interval
 *
 */
#define timer_value(timer)						\
__extension__({								\
	struct timeval __tmptv;						\
	gettimeofday(&__tmptv, NULL);					\
	(timer_uint) __tmptv.tv_sec * 1000000 + __tmptv.tv_usec - timer;\
})
