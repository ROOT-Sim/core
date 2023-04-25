/**
 * @file test/framework/rng.c
 *
 * @brief Pseudo random number generator for tests
 *
 * An acceptable-quality pseudo random number generator to be used in tests
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <stdint.h>

typedef __uint128_t test_rng_state;

extern void rng_init(test_rng_state *rng_state, test_rng_state initseq);
extern uint64_t rng_random_u(test_rng_state *rng_state);
extern double rng_random(test_rng_state *rng_state);
extern uint64_t rng_random_range(test_rng_state *rng_state, uint64_t n);
extern int rng_ks_test(uint32_t n_samples, double (*sample)(void));
