/**
 * @file mm/mm.c
 *
 * @brief Memory Manager main header
 *
 * Memory Manager main header
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <mm/mm.h>

extern void *mm_alloc(size_t mem_size);
extern void *mm_realloc(void *ptr, size_t mem_size);
extern void mm_free(void *ptr);
