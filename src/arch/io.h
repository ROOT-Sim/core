#pragma once

#include <arch/platform.h>

#include <stdbool.h>
#include <stdio.h>

#ifdef __POSIX
typedef FILE *io_file_t;
#endif

#ifdef __WINDOWS
typedef HANDLE io_file_t;
#endif

#define IO_TIME_BUFFER_LEN 26

extern bool io_terminal_can_colorize(void);
extern void io_local_time_get(char res[IO_TIME_BUFFER_LEN]);
extern io_file_t io_file_tmp_get(void);
extern int io_file_append(io_file_t f, const void *data, size_t data_size);
extern int io_file_process(io_file_t f, void *buf, size_t buf_size,
			   void (*proc_fnc)(size_t, void *), void *proc_fnc_arg);
