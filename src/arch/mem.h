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

#include <stddef.h>

int mem_stat_setup(void);
size_t mem_stat_rss_max_get(void);
size_t mem_stat_rss_current_get(void);
