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

#include <lib/random/random.h>
#include <lib/state/state.h>
#include <lib/topology/topology.h>

#ifdef RETRACTABILITY
#include <lib/retractable/retractable.h>
#endif

struct lib_ctx {
	// random library
	uint64_t rng_s[4];
	double unif;
	
#ifdef RETRACTABILITY
	// retractability library
	simtime_t r_ts; // Timestamp of retractable msg
	uint_fast32_t r_e_type; // Event type
	//char* r_payload;
#endif

	// todo remove
	void *state_s;
};

extern void lib_global_init(void);
extern void lib_global_fini(void);

extern void lib_lp_init(void);
extern void lib_lp_fini(void);

extern void lib_lp_init_pr(void);
extern void lib_lp_fini_pr(void);

