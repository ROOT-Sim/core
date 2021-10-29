/**
 * @file log/log.c
 *
 * @brief Logging library
 *
 * This library can be used to produce logs during simulation runs.
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <log/log.h>

#include <arch/io.h>

#include <stdarg.h>
#include <stdio.h>

/// The minimum log level of the messages to display
visible int log_level = LOG_LEVEL;
/// If set, uses color codes to color the log outputs
bool log_colored;

/// The textual representations and the color codes of the logging levels
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

/**
 * @brief Logs a message. For internal use: log_log() should be used instead
 * @param level the importance level of the message to log
 * @param file the file name where this function is being called
 * @param line the line number where this function is being called
 * @param fmt a printf-style format string for the message to log
 * @param ... the list of arguments to fill in the format string @a fmt
 */
visible void _log_log(int level, const char *file, unsigned line, const char *fmt, ...)
{
	va_list args;

	char time_string[IO_TIME_BUFFER_LEN];
	io_local_time_get(time_string);

#if LOG_CAN_LOG_AT_BUILD(LOG_DEBUG)
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
#else
	(void) file;
	(void) line;
	if(log_colored) {
		fprintf(
			stderr,
			"%s %s%-5s\x1b[0m\x1b[0m ",
			time_string,
			levels[level].color,
			levels[level].name
		);
	} else {
		fprintf(
			stderr,
			"%s %-5s ",
			time_string,
			levels[level].name
		);
	}
#endif

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	fprintf(stderr, "\n");
	fflush(stderr);
}

/**
 * @brief Prints a fancy ROOT-Sim logo on the terminal
 */
void log_logo_print(void)
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
