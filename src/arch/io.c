#include <arch/io.h>

#include <core/core.h>

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#ifdef __POSIX

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

io_file_t io_file_tmp_get(void)
{
	return tmpfile();
}

int io_file_append(io_file_t f, const void *data, size_t data_size)
{
	return -(fwrite(data, data_size, 1, f) != 1);
}

int io_file_process(io_file_t f, void *buffer, size_t buffer_size,
		    void (*proc_fnc)(size_t, void *), void *proc_fnc_arg)
{
	if (fseek(f, 0, SEEK_SET))
		return -1;

	while (1) {
		size_t res = fread(buffer, 1, buffer_size, f);
		if (res != buffer_size) {
			if (res)
				proc_fnc(res, proc_fnc_arg);
			return - (!feof(f));
		}
		proc_fnc(res, proc_fnc_arg);
	}
}

#endif

#ifdef __WINDOWS

#include <stdio.h>
#include <io.h>

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

io_file_t io_file_tmp_get(void)
{
	TCHAR tmp_folder_path[MAX_PATH + 1];
	if (!GetTempPath(MAX_PATH + 1, tmp_folder_path))
		return NULL;

	TCHAR tmp_path[MAX_PATH];
	if (!GetTempFileName(tmp_folder_path, TEXT("ROOTSIM"), 0, tmp_path))
		return NULL;

	HANDLE ret = CreateFile((LPTSTR) tmp_path, GENERIC_READ | GENERIC_WRITE,
				0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY
				| FILE_FLAG_DELETE_ON_CLOSE, NULL);

	return ret == INVALID_HANDLE_VALUE ? NULL : ret;
}

int io_file_append(io_file_t f, const void *data, size_t data_size)
{
	LDWORD written;
	return -(!WriteFile(f, data, data_size, &written, NULL));
}

int io_file_process(io_file_t f, void *buffer, size_t buffer_size,
		    void (*proc_fnc)(size_t, void *), void *proc_fnc_arg)
{
	if (SetFilePointer(f, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
		return -1;

	while (1) {
		LDWORD res;
		if (ReadFile(f, buffer, buffer_size, &read, NULL))
			return -1;

		if (res != buffer_size) {
			if (res)
				proc_fnc(res, proc_fnc_arg);
			return 0;
		}
		proc_fnc(res, proc_fnc_arg);
	}
}

#endif
