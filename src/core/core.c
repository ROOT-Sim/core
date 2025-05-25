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

__thread tid_t tid;
nid_t n_nodes = 1;
nid_t nid;

const char *core_version = ROOTSIM_VERSION;
