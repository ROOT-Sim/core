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

#include <stdio.h>

void vlogger(unsigned level, char *file, unsigned line, const char *fmt, ...);

/**
 * @brief Produce a log message
 * @param level the logging level associated to the message
 * @param fmt the printf-style format string of the message
 * @param ... the format string arguments if needed
 */
#define logger(level, fmt, ...) vlogger(level, __FILE__, __LINE__, fmt __VA_OPT__(,) __VA_ARGS__)

extern void log_init(FILE *file);
