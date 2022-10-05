/**
 * @file lib/lib.h
 *
 * @brief Model library main header
 *
 * This is the main header to initialize core model libraries.
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <stdbool.h>

#include <lib/random/random.h>
#include <lib/state/state.h>

/// Per-LP structure for core libraries
struct lib_ctx {
	/// The current random number generator state
	uint64_t rng_s[4];
	/// Normal deviates are computed in pairs; this member keeps the second generated pair
	double unif;
	/// This flag tells whether unif member is keeping a valid deviate
	bool has_normal;
	/// The pointer set by the model with the SetState() API call
	void *state_s;
};

extern void lib_global_init(void);
extern void lib_global_fini(void);

extern void lib_lp_init(void);
extern void lib_lp_fini(void);
