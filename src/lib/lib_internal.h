/**
* @file lib/lib_internal.h
*
* @brief Internal core libraries
*
* This is a commodity header to plug internal ROOT-Sim libraries.
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

#include <lib/lib.h>

#ifdef NEUROME_SERIAL
#include <serial/serial.h>
#define l_s_p			(&cur_lp->ls)
#define l_s_m_p 		(&cur_lp->lsm)
#define current_lid		(cur_lp - lps)
#else
#include <lp/lp.h>
#define l_s_p			(&current_lp->ls)
#define l_s_m_p 		(current_lp->lsm_p)
#define current_lid		(current_lp - lps)
#endif

#ifdef NEUROME_INCREMENTAL
#include <mm/model_allocator.h>
#define mark_written(ptr, size)	__write_mem(ptr, size)
#else
#define mark_written(ptr, size)
#endif
