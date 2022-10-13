/**
 * @file mm/dymelor/checkpoint.h
 *
 * @brief Checkpointing capabilities for DyMeLoR
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <mm/dymelor/dymelor.h>

#include <stdint.h>

struct dymelor_state_checkpoint {
	uint_fast32_t used_mem;
	unsigned char data[];
};

struct dymelor_area_checkpoint {
	unsigned i;
	uint_least32_t chunk_cnt;
	unsigned char data[];
};

_Static_assert(alignof(struct dymelor_area_checkpoint) <= sizeof(uint_least32_t),
    "Adjacent checkpoints may not satisfy memory access alignment requirements");

extern struct dymelor_state_checkpoint *dymelor_checkpoint_full_take(const struct dymelor_state *ctx);
extern void dymelor_checkpoint_full_restore(struct dymelor_state *ctx, const struct dymelor_state_checkpoint *ckpt);
extern void dymelor_checkpoint_trim_to(struct dymelor_state *ctx, const struct dymelor_state_checkpoint *state_ckpt);
