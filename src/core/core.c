/**
 * @file core/core.c
 *
 * @brief Core ROOT-Sim functionalities
 *
 * Core ROOT-Sim functionalities
 *
 * SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <core/core.h>

_Thread_local rid_t rid;
nid_t n_nodes = 1;
nid_t nid;

char *core_version = ROOTSIM_VERSION;
