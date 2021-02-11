/**
* @file log/log.h
*
* @brief Logging library
*
* This library can be used to produce logs during simulation runs.
*
* @copyright
* Copyright (C) 2008-2021 HPDCS Group
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

#include <stdbool.h>

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_TRACE
#endif

extern int log_level;
extern bool log_colored;

#define LOG_TRACE 	0
#define LOG_DEBUG 	1
#define LOG_INFO 	2
#define LOG_WARN 	3
#define LOG_ERROR 	4
#define LOG_FATAL 	5

#define log_is_lvl(level) ((level) >= LOG_LEVEL && (level) >= log_level)

#define log_log(lvl, ...)						\
	do {								\
		if(log_is_lvl(lvl))					\
			_log_log(lvl, __FILE__, __LINE__, __VA_ARGS__);	\
	} while(0)

void _log_log(int level, const char *file, unsigned line, const char *fmt, ...);

void log_logo_print(void);
