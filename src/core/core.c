/**
 * @file core/core.c
 *
 * @brief Core ROOT-Sim functionalities
 *
 * Core ROOT-Sim functionalities
 *
 * SPDX-FileCopyrightText: 2008-2023 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <core/core.h>
#include <core/compiler.h>

__thread rid_t rid;
nid_t n_nodes = 1;
nid_t nid;

char *core_version = ROOTSIM_VERSION;
