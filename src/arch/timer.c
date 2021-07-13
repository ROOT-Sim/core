/**
 * @file arch/timer.c
 *
 * @brief Timers
 *
 * This module defines the timers which the simulator uses to monitor its
 * internal behaviour
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <arch/timer.h>

#ifdef __WINDOWS
timer_uint timer_perf_freq = 0;
#endif

/**
 * @brief Gets a new starting point for an time interval measure
 * @return a timer_uint value, a not meaningful value by itself
 *
 * The returned value can be used in conjunction with timer_value() to measure a
 * time interval with microsecond resolution
 */
extern timer_uint timer_new(void);

/**
 * @brief Computes a time interval measure using a previous timer_uint value
 * @param start a timer_uint value obtained from a previous timer_new() call
 * @return a timer_uint value, the count of microseconds of the time interval
 */
extern timer_uint timer_value(timer_uint start);
