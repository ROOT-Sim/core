/**
 * @file lib/lib.c
 *
 * @brief Model library main module
 *
 * This is the main module to initialize core model libraries.
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <lib/lib.h>

#include <lib/lib_internal.h>

void lib_global_init(void)
{
	topology_global_init();
}

void lib_global_fini(void)
{

}

void lib_lp_init(void)
{
	random_lib_lp_init();
	state_lib_lp_init();
//~ #ifdef RETRACTABILITY
	//~ retractable_lib_lp_init();
//~ #endif

}

void lib_lp_fini(void)
{

}
