/**
 * @file log/logger.h
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
#include <stdio.h>

#include <ROOT-Sim.h>

void vlogger(unsigned level, char *file, unsigned line, const char *fmt, ...);

/**
 * @fn logger(lvl, fmt, ...)
 * @brief Produces a logger
 * @param lvl the logging level associated to the message
 * @param ... a printf-style format string followed by its arguments if needed
 */
#define logger(level, fmt, ...) vlogger(level, __FILE__, __LINE__, fmt, #__VA_ARGS__)

extern void log_init(FILE *file);
