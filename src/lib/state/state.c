/**
 * @file lib/state/state.c
 *
 * @brief LP main state management
 *
 * This library is responsible for allowing LPs to set their state entry
 * point and change it at runtime.
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <lib/state/state.h>
#include "lp/lp.h"

/**
 * @brief Set the LP simulation state main pointer
 * @param state The state pointer to be passed to ProcessEvent() for the invoker LP
 */
void SetState(void *state)
{
	struct lib_ctx *ctx = lib_ctx_get();
	ctx->state_s = state;
}

void state_lib_lp_init(void)
{
	lib_ctx_get()->state_s = NULL;
}
