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

struct dymelor_ctx_checkpoint {
	unsigned area_cnt;
	unsigned char data[];
};

struct dymelor_area_checkpoint {
	unsigned i;
	uint_least32_t chunk_cnt;
	unsigned char data[];
};

extern struct dymelor_ctx_checkpoint *dymelor_checkpoint_full_take(const struct dymelor_state *ctx);
extern void dymelor_checkpoint_full_restore(struct dymelor_state *ctx, const struct dymelor_ctx_checkpoint *ckpt);
