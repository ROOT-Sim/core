/**
 * @file lib/state/state.c
 *
 * @brief LP main state management
 *
 * This library is responsible for allows LPs to set their state entry
 * point and change it at runtime.
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
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
