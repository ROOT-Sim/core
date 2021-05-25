/**
 * @file arch/mem.h
 *
 * @brief Platform specific memory utilities
 *
 * This header exposes some memory related utilities such as memory statistics
 * retrieval in a platform independent way
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <arch/platform.h>

#include <stddef.h>

#ifdef __WINDOWS
#include <malloc.h>
#define mem_aligned_alloc(align, mem_size) _aligned_malloc(mem_size, align)
#define mem_aligned_free(mem) _aligned_free(mem)
#else
#include <stdlib.h>
#define mem_aligned_alloc(align, mem_size) aligned_alloc(align, mem_size)
#define mem_aligned_free(mem) free(mem)
#endif


extern int mem_stat_setup(void);
extern size_t mem_stat_rss_max_get(void);
extern size_t mem_stat_rss_current_get(void);
