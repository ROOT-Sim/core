/**
 * @file arch/mem.h
 *
 * @brief Platform specific memory utilities
 *
 * This header exposes some memory related utilities such as memory statistics
 * retrieval in a platform independent way
 *
 * SPDX-FileCopyrightText: 2008-2023 HPDCS Group <rootsim@googlegroups.com>
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
/// An OS-dependent function to allocate aligned memory
#define mem_aligned_alloc(align, mem_size) aligned_alloc(align, mem_size)
/// An OS-dependent function to free aligned memory
#define mem_aligned_free(mem) free(mem)
#endif
/// Deterministic allocations make use of 2 MiB large pages; should be good for x86-64 and AArch64
#define MEM_DETERMINISTIC_PAGE_SIZE (1UL << 21)

extern int mem_deterministic_alloc(void *ptr, size_t mem_size);
extern void mem_deterministic_free(void *ptr, size_t mem_size);

extern int mem_stat_setup(void);
extern size_t mem_stat_rss_max_get(void);
extern size_t mem_stat_rss_current_get(void);
