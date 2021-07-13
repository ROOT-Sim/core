/**
 * @file test/test_stubs.c
 *
 * @brief Test stubs source
 *
 * The source of the stubs used to build the tests which use external symbols
 * (mostly the ones declared in log/log.h, mm/mm.h and core/core.h)
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <test.h>

#include <mm/mm.h>

#ifdef ROOTSIM_MPI
nid_t nid;
nid_t n_nodes = 1;
#endif
lp_id_t n_lps;
rid_t n_threads;
__thread rid_t rid;

int log_level;

extern void *mm_aligned_alloc(size_t alignment, size_t mem_size);
extern void *mm_alloc(size_t mem_size);
extern void *mm_realloc(void *ptr, size_t mem_size);
extern void mm_free(void *ptr);

void _log_log(int level, const char *file, unsigned line, const char *fmt, ...)
{
	(void) level;
	(void) file;
	(void) line;
	(void) fmt;
}

void log_logo_print(void) {}
