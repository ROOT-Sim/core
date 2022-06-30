/**
 * @file log/log.h
 *
 * @brief Logging library
 *
 * This library can be used to produce logs during simulation runs.
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <stdio.h>

void vlogger(unsigned level, char *file, unsigned line, const char *fmt, ...);

/**
 * @brief Produce a log message
 * @param level the logging level associated to the message
 * @param ... the format string followed by its arguments if needed
 */
#define logger(level, ...) vlogger(level, __FILE__, __LINE__, __VA_ARGS__)

extern void log_init(FILE *file);
