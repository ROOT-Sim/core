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

/**
 * @brief Gets a new starting point for an time interval measure
 * @return a timer_uint value, a not meaningful value by itself
 *
 * The returned value can be used in conjunction with timer_value() to measure a
 * time interval with microsecond resolution
 */
static inline timer_uint timer_new(void)
{
	struct timeval tmptv;
	gettimeofday(&tmptv, NULL);
	return (timer_uint)tmptv.tv_sec * 1000000U + tmptv.tv_usec;
}

/**
 * @brief Computes a time interval measure using a previous timer_uint value
 * @param start a timer_uint value obtained from a previous timer_new() call
 * @return a timer_uint value, the count of microseconds of the time interval
 */
static inline timer_uint timer_value(timer_uint start)
{
	return timer_new() - start;
}

#endif

#ifdef __WINDOWS
#include <windows.h>

static timer_uint timer_perf_freq = 0;

static inline timer_uint timer_new(void)
{
	LARGE_INTEGER start_time;
	QueryPerformanceCounter(&start_time);
	return (timer_uint)start_time.QuadPart;
}


static inline timer_uint timer_value(timer_uint start)
{
	if(unlikely(timer_perf_freq == 0)) {
		LARGE_INTEGER perf;
		QueryPerformanceFrequency(&perf);
		timer_perf_freq = perf.QuadPart;
	}
	return (timer_new() - start) * 1000000U / timer_perf_freq;
}

#endif

#if defined(__x86_64__) || defined(__i386__)
#ifdef __WINDOWS
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

/**
 * @brief Starts a high resolution, CPU dependent time interval measure
 * @return a timer_uint value, a not meaningful value by itself
 *
 * The returned value can be used in conjunction with timer_hr_value() to
 * measure a time interval with unspecified resolution
 */
static inline timer_uint timer_hr_new(void)
{
	return __rdtsc();
}

#else

static inline timer_uint timer_hr_new(void)
{
	return timer_new();
}

#endif

/**
 * @brief Computes a time interval measure using a previous timer_uint value
 * @param start a timer_uint value obtained from a previous timer_hr_new() call
 * @return a timer_uint value, a measure of the time interval
 */
static inline timer_uint timer_hr_value(timer_uint start)
{
	return timer_hr_new() - start;
}
