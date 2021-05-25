/**
 * @file arch/io.c
 *
 * @brief Generic input-output facilities
 *
 * This module defines architecture independent input-oputput facilities for the
 * use in the simulator
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <arch/io.h>

/**
 * @fn io_terminal_can_colorize(void)
 * @brief Determines if stdout supports colored text
 * @return true if colors escape sequences can be used, false otherwise
 */

/**
 * @fn io_local_time_get(char res[IO_TIME_BUFFER_LEN])
 * @brief Fills in a formatted string of the current time
 * @param res a pointer to a memory area with at least IO_TIME_BUFFER_LEN chars
 *
 * The resulting string is written in @a res.
 */

/**
 * @fn io_file_tmp_get(void)
 * @brief Creates a temporary file
 * @return a temporary file, an opaque object to be used in this module
 */

#include <core/core.h>

#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __POSIX

#include <unistd.h>

bool io_terminal_can_colorize(void)
{
	return isatty(STDERR_FILENO);
}

void io_local_time_get(char res[IO_TIME_BUFFER_LEN])
{
	time_t t = time(NULL);
	struct tm *loc_t = localtime(&t);
	strftime(res, IO_TIME_BUFFER_LEN, "%H:%M:%S", loc_t);
}

FILE *io_file_tmp_get(void)
{
	return tmpfile();
}

#endif

#ifdef __WINDOWS

#include <fcntl.h>
#include <io.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

bool io_terminal_can_colorize(void)
{
	HANDLE term = GetStdHandle(STD_ERROR_HANDLE);
	if (term == NULL || term == INVALID_HANDLE_VALUE)
		return false;

	DWORD cmode;
	if (!GetConsoleMode(term, &cmode))
		return false;

	return SetConsoleMode(term, cmode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}

void io_local_time_get(char res[IO_TIME_BUFFER_LEN])
{
	struct tm loc_t;
	__time64_t long_time;
	_time64(&long_time);

	_localtime64_s(&loc_t, &long_time);
	strftime(res, IO_TIME_BUFFER_LEN, "%H:%M:%S", &loc_t);
}

FILE *io_file_tmp_get(void)
{
	TCHAR tmp_folder_path[MAX_PATH + 1];
	if (!GetTempPath(MAX_PATH + 1, tmp_folder_path))
		return NULL;

	TCHAR tmp_path[MAX_PATH];
	if (!GetTempFileName(tmp_folder_path, TEXT("ROOTSIM"), 0, tmp_path))
		return NULL;

	SECURITY_ATTRIBUTES tmp_sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};
	HANDLE file_handle = CreateFile(
			tmp_path, GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			&tmp_sa, CREATE_ALWAYS,
			FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, NULL);
	if (file_handle == INVALID_HANDLE_VALUE)
		return NULL;

	int fd = _open_osfhandle((intptr_t)file_handle, _O_RDWR);
	if (fd == -1)
		return NULL;

	return _fdopen(fd, "rb+");
}

#endif
