/**
 * @file lib/random/random.h
 *
 * @brief Random Number Generators
 *
 * Piece-Wise Deterministic Random Number Generators.
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <core/core.h>

#include <stdint.h>

/// The container for the pseudo random number generator context
struct rng_ctx {
	/// The current PRNG state
	uint64_t state[4];
};

extern void random_lib_lp_init(lp_id_t lp_id, struct rng_ctx *rng_ctx);
