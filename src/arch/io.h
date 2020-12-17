#pragma once

#include <arch/platform.h>

#include <stdio.h>

#ifdef __POSIX

#include <unistd.h>
typedef FILE *io_tmp_file_t;
#define io_terminal_can_colorize() isatty(STDERR_FILENO)

#endif

#ifdef __WINDOWS

#include <io.h>
typedef HANDLE io_tmp_file_t;
#define io_terminal_can_colorize() (_fileno(stderr) > 0 && _isatty(_fileno(stderr)))

#endif

#define IO_TIME_BUFFER_LEN 26

extern void io_local_time_get(char res[IO_TIME_BUFFER_LEN]);


extern io_tmp_file_t io_tmp_file_get(void);
extern int io_tmp_file_append(io_tmp_file_t f, const void *data, size_t data_size);
extern int io_tmp_file_process(io_tmp_file_t f, void *buffer, size_t buffer_size,
			       void (*proc_fnc)(size_t, void *), void *proc_fnc_arg);
