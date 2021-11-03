/**
 * @file log/log.h
 *
 * @brief Logging library
 *
 * This library can be used to produce logs during simulation runs.
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <stdbool.h>

#ifndef LOG_LEVEL
/// The minimum logging level supported at compile time
/** Compiler optimizations remove log calls with lower level than this one */
#define LOG_LEVEL LOG_TRACE
#endif

#define LOG_CAN_LOG_AT_BUILD(l) (l >= LOG_LEVEL)

extern int log_level;
extern bool log_colored;

/// The logging level reserved to very low priority messages
#define LOG_TRACE 	0
/// The logging level reserved to useful debug messages
#define LOG_DEBUG 	1
/// The logging level reserved to useful runtime messages
#define LOG_INFO 	2
/// The logging level reserved to unexpected, non deal breaking conditions
#define LOG_WARN 	3
/// The logging level reserved to unexpected, problematic conditions
#define LOG_ERROR 	4
/// The logging level reserved to unexpected, fatal conditions
#define LOG_FATAL 	5

/**
 * @brief Checks if a logging level is being processed
 * @param lvl the logging level to check
 * @return true if @a level is being process, false otherwise
 */
#define log_can_log(lvl) ((lvl) >= LOG_LEVEL && (lvl) >= log_level)

/**
 * @fn log_log(lvl, ...)
 * @brief Produces a log
 * @param lvl the logging level associated to the message
 * @param ... a printf-style format string followed by its arguments if needed
 */

#if LOG_CAN_LOG_AT_BUILD(LOG_DEBUG)

#define log_log(lvl, ...)						\
	do {								\
		if(log_can_log(lvl))					\
			_log_log(lvl, __FILE__, __LINE__, __VA_ARGS__);	\
	} while(0)
#else

#define log_log(lvl, ...)						\
	do {								\
		if(log_can_log(lvl))					\
			_log_log(lvl, NULL, 0, __VA_ARGS__);		\
	} while(0)

#endif

void _log_log(int level, const char *file, unsigned line, const char *fmt, ...);

void log_logo_print(void);
