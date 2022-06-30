/**
 * @file gvt/fossil.h
 *
 * @brief Housekeeping operations
 *
 * In this module all the housekeeping operations related to GVT computation phase
 * are present.
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <core/core.h>
#include <lp/lp.h>

extern void fossil_lp_on_gvt(struct lp_ctx *lp, simtime_t current_gvt);
