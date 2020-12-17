/**
* @file log/log.c
*
* @brief Logging library
*
* This library can be used to produce logs during simulation runs.
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
#include <log/log.h>

#include <arch/io.h>

#include <stdarg.h>
#include <stdio.h>

int log_level = LOG_LEVEL;
bool log_colored;

static const struct {
	const char *name;
	const char *color;
} levels[] = {
		[LOG_TRACE] = {.name = "TRACE", .color = "\x1b[94m"},
		[LOG_DEBUG] = {.name = "DEBUG", .color = "\x1b[36m"},
		[LOG_INFO] = {.name = "INFO", .color = "\x1b[32m"},
		[LOG_WARN] = {.name = "WARN", .color = "\x1b[33m"},
		[LOG_ERROR] = {.name = "ERROR", .color = "\x1b[31m"},
		[LOG_FATAL] = {.name = "FATAL", .color = "\x1b[35m"}
};


void _log_log(int level, const char *file, unsigned line, const char *fmt, ...)
{
	va_list args;

	char time_string[IO_TIME_BUFFER_LEN];
	io_local_time_get(time_string);

	if(log_colored) {
		fprintf(
				stderr,
				"%s %s%-5s\x1b[0m \x1b[90m%s:%u:\x1b[0m ",
				time_string,
				levels[level].color,
				levels[level].name,
				file,
				line
		);
	} else {
		fprintf(
				stderr,
				"%s %-5s %s:%u: ",
				time_string,
				levels[level].name,
				file,
				line
		);
	}

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	fprintf(stderr, "\n");
	fflush(stderr);
}

void log_logo_print()
{
	if(log_colored) {
		fprintf(stderr, "\x1b[94m   __ \x1b[90m __   _______   \x1b[94m  _ \x1b[90m       \n");
		fprintf(stderr, "\x1b[94m  /__)\x1b[90m/  ) /  ) /  __ \x1b[94m ( `\x1b[90m . ___ \n");
		fprintf(stderr, "\x1b[94m / \\ \x1b[90m(__/ (__/ (      \x1b[94m._)\x1b[90m / / / )\n");
		fprintf(stderr, "\x1b[0m\n");
	} else {
		fprintf(stderr, "  __   __   _______    _        \n");
		fprintf(stderr, " /__) /  ) /  ) /  __ ( ` . ___ \n");
		fprintf(stderr, "/ \\  (__/ (__/ (     ._) / / / )\n");
		fprintf(stderr, "\n");
	}
}
