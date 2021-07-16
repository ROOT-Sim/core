/**
 * @file lib/lib.h
 *
 * @brief Model library main header
 *
 * This is the main header to initialize core model libraries.
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <core/visibility.h>

#include <lib/random/random.h>
#include <lib/state/state.h>
#include <lib/topology/topology.h>

struct lib_ctx {
	// random library
	uint64_t rng_s[4];
	double unif;
	// todo remove
	void *state_s;
};

#ifdef ROOTSIM_MOD_BUILD
visible extern void lib_global_init(void);
visible extern void lib_global_fini(void);

visible extern void lib_lp_init(void);
visible extern void lib_lp_fini(void);

visible extern void lib_lp_init_pr(void);
visible extern void lib_lp_fini_pr(void);
#else
want_visible extern void lib_global_init(void);
want_visible extern void lib_global_fini(void);

want_visible extern void lib_lp_init(void);
want_visible extern void lib_lp_fini(void);

want_visible extern void lib_lp_init_pr(void);
want_visible extern void lib_lp_fini_pr(void);
#endif
