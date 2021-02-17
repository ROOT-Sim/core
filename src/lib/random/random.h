/**
 * @file lib/random/random.h
 *
 * @brief Random Number Generators
 *
 * Piece-Wise Deterministic Random Number Generators.
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <stdint.h>

extern void random_lib_lp_init(void);

extern double Random(void);
extern uint64_t RandomU64(void);
extern double Expent(double mean);
extern double Normal(void);
