/**
* @file lib/state/state.c
*
* @brief LP main state management
*
* This library is responsible for allows LPs to set their state entry
* point and change it at runtime.
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
#include <lib/state/state.h>

#include <lib/lib_internal.h>

void SetState(void *state)
{
	struct lib_ctx *ctx = lib_ctx_get();
	ctx->state_s = state;
}

void state_lib_lp_init(void)
{
	lib_ctx_get()->state_s = NULL;
}
