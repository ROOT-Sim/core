/**
 * @file log/file.h
 *
 * @brief File utilities
 *
 * Some file utility functions
 *
 * SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <core/core.h>
#include <log/log.h>

#include <stdio.h>

void *file_memory_load(FILE *f, int64_t *f_size_p);
FILE *file_open(const char *open_type, const char *fmt, ...);

/**
 * @brief Write the content of a memory buffer in a file
 * @param[in] f the file to write
 * @param[in] data a pointer to the memory buffer containing the data to write to the file
 * @param[in] data_size the size of the memory buffer pointed by @p data
 *
 * Very stupid convenience function which does basic error handling on the actual fwrite() call.
 */
static inline void file_write_chunk(FILE *f, const void *data, size_t data_size)
{
	if(unlikely(fwrite(data, data_size, 1, f) != 1 && data_size))
		logger(LOG_ERROR, "Error during disk write!");
}
