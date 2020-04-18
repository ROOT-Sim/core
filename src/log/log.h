#pragma once

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_TRACE
#endif

extern int log_level;
extern _Bool log_colored;

#define LOG_TRACE 	0
#define LOG_DEBUG 	1
#define LOG_INFO 	2
#define LOG_WARN 	3
#define LOG_ERROR 	4
#define LOG_FATAL 	5

#define log_is_lvl(level) ((level) >= LOG_LEVEL && (level) >= log_level)

#ifndef NEUROME_TEST

#define log_log(lvl, ...)						\
	do {								\
		if(log_is_lvl(lvl))					\
			_log_log(lvl, __FILE__, __LINE__, __VA_ARGS__);	\
	} while(0)

#else

#define log_log(lvl, ...)

#endif

void _log_log(int level, const char *file, unsigned line, const char *fmt, ...);

void log_logo_print(void);

#ifndef NEUROME_LOG_INTERNAL
#pragma GCC poison _log_log
#endif
