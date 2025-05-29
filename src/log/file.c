/**
 * @file log/file.c
 *
 * @brief File utilities
 *
 * Some file utility functions
 *
 * SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <log/file.h>

#include <mm/mm.h>

#include <stdarg.h>

/**
 * @brief Load a whole file content in a mm_alloc-ed memory buffer
 * @param[in] f the file to load
 * @param[out] f_size_p a pointer to a variable which will contain with the loaded file size
 * @return a mm_alloc-ed buffer filled in with the file content, or NULL if a read error happened
 */
void *file_memory_load(FILE *f, int64_t *f_size_p)
{
	fseek(f, 0, SEEK_END);
	const long f_size = ftell(f); // FIXME: may fail horribly for files bigger than 2 GB
	fseek(f, 0, SEEK_SET);
	void *ret = mm_alloc(f_size);
	if(fread(ret, f_size, 1, f) != 1) {
		mm_free(ret);
		*f_size_p = 0;
		return NULL;
	}
	*f_size_p = f_size;
	return ret;
}

/**
 * @brief A version of fopen() which accepts a printf style format string
 * @param open_type a string which controls how the file is opened (see fopen())
 * @param fmt the file name expressed as a printf style format string
 * @param ... the list of additional arguments used in @a fmt (see printf())
 */
FILE *file_open(const char *open_type, const char *fmt, ...)
{
	va_list args, args_cp;
	va_start(args, fmt);
	va_copy(args_cp, args);

	const size_t l = vsnprintf(NULL, 0, fmt, args_cp) + 1;
	va_end(args_cp);

	char *f_name = mm_alloc(l);
	vsnprintf(f_name, l, fmt, args);
	va_end(args);

	FILE *ret = fopen(f_name, open_type);
	if(ret == NULL)
		logger(LOG_ERROR, "Unable to open \"%s\" in %s mode", f_name, open_type);

	mm_free(f_name);
	return ret;
}
