/**
 * @file core/core.c
 *
 * @brief Core ROOT-Sim functionalities
 *
 * Core ROOT-Sim functionalities
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <core/core.h>

lp_id_t n_lps;

rid_t n_threads;
__thread rid_t rid;
#ifdef ROOTSIM_MPI
nid_t n_nodes = 1;
rid_t n_threads;
nid_t nid;
#endif
