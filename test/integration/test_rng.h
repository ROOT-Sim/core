/**
 * @file test/integration/test_rng.h
 *
 * @brief Simple rollbackable RNG for tests
 *
 * SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#ifndef TEST_RNG_H
#define TEST_RNG_H

#include <stdint.h>

struct test_rng_state {
	__uint128_t seed;
};

static inline void test_rng_set_seed(const __uint128_t seed, struct test_rng_state *state)
{
	state->seed = ((seed) << 1u) | 1u;
}

static inline double test_rng_random(struct test_rng_state *state)
{
	const __uint128_t multiplier = (((__uint128_t)0x0fc94e3bf4e9ab32ULL) << 64) + 0x866458cd56f5e605ULL;
	state->seed *= multiplier;
	const uint64_t ret = state->seed >> 64u;
	return (double)ret / (double)UINT64_MAX;
}

#endif /* TEST_RNG_H */
