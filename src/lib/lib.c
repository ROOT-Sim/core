/**
* @file lib/lib.c
*
* @brief Model library main module
*
* This is the main module to initialize core model libraries.
*
* @copyright
* Copyright (C) 2008-2020 HPDCS Group
* https://hpdcs.github.io
*
* This file is part of ROOT-Sim (ROme OpTimistic Simulator).
*
* ROOT-Sim is free software; you can redistribute it and/or modify it under the
* terms of the GNU General Public License as published by the Free Software
* Foundation; only version 3 of the License applies.
*
* ROOT-Sim is distributed in the hope that it will be useful, but WITHOUT ANY
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
* A PARTICULAR PURPOSE. See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with
* ROOT-Sim; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include <lib/lib.h>

#include <lib/lib_internal.h>

static char lib_doc[] = "NeuRome model development libraries";
// this isn't needed (we haven't got non option arguments to document)
static char lib_args_doc[] = "";

static const struct argp_option lib_argp_options[] = {
	{0}
};

static error_t lib_parse_opt (int key, char *arg, struct argp_state *state)
{
	(void)key;
	(void)arg;
	(void)state;
	// TODO parsing options for model's library
	return ARGP_ERR_UNKNOWN;
}

const struct argp lib_argp = {lib_argp_options, lib_parse_opt, lib_args_doc, lib_doc, 0, 0, 0};

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
}

void lib_lp_fini(void)
{

}
