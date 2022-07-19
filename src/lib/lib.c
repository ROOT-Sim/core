/**
 * @file lib/lib.c
 *
 * @brief Model library main module
 *
 * This is the main module to initialize core model libraries.
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <lib/lib.h>

/**
 * @brief Initialize the model library implemented in the core
 * @todo This part shall be removed
 */
void lib_global_init(void) {}

/**
 * @brief Finalize the model library implemented in the core
 * @todo This part shall be removed
 */
void lib_global_fini(void) {}

/**
 * @brief Initialize all libraries to support the lifetime of the LPs
 * @todo This part shall be removed
 */
void lib_lp_init(void)
{
	random_lib_lp_init();
	retractable_lib_lp_init();
	state_lib_lp_init();
}

/**
 * @brief Finalize all libraries to support the lifetime of the LPs
 * @todo This part shall be removed
 */
void lib_lp_fini(void) {}
