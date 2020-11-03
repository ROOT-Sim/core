/**
 * @file core/core.c
 *
 * @brief Core ROOT-Sim functionalities
 *
 * Core ROOT-Sim functionalities
 *
 * @copyright
 * Copyright (C) 2008-2020 HPDCS Group
 * https://rootsim.github.io/core
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
#include <core/core.h>

#include <stdatomic.h>
#include <setjmp.h>

lp_id_t n_lps;

#ifndef ROOTSIM_SERIAL

nid_t n_nodes = 1;
rid_t n_threads;
nid_t nid;
__thread rid_t rid;

static __thread jmp_buf exit_jmp_buf;

void core_init(void)
{
	static atomic_uint rid_helper = 0;
	rid = atomic_fetch_add_explicit(&rid_helper, 1U, memory_order_relaxed);

	int code = setjmp(exit_jmp_buf);
	if(code){
		//todo abort mission!
	}
}

void core_abort(void)
{
	// BUG: Undefined behavior: exit_jmp_buf was set in a function which returned
	longjmp(exit_jmp_buf, -1);
}

#endif
