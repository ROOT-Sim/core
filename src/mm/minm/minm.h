/**
* @file mm/minm/minm.h
*
* @brief A minimalistic memory allocator for simulation models
*
* @copyright
* Copyright (C) 2008-2020 HPDCS Group
* https://hpdcs.github.io
*
* This file is part of ROOT-Sim (ROme OpTimistic Simulator).
*
* ROOT-Sim is free software; you can redistribute it and/or modify it under the
* terms of the GNU General Public License as published by the Free Software
* Foundation; only version 3 of the License applies.
*
* ROOT-Sim is distributed in the hope that it will be useful, but WITHOUT ANY
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
* A PARTICULAR PURPOSE. See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with
* ROOT-Sim; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#pragma once

#include <core/core.h>
#include <datatypes/array.h>

#ifdef ROOTSIM_INCREMENTAL
#error "The minimal model allocator doesn't support incremental checkpointing"
#endif

#define B_CACHE_LINES 1

struct _mm_chunk {
	unsigned char m[B_CACHE_LINES * CACHE_LINE_SIZE];
};

struct mm_state {
	dyn_array(struct _mm_chunk ) chunks;
	array_count_t last_ref_i;
};
