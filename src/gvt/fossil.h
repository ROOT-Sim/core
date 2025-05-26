/**
 * @file gvt/fossil.h
 *
 * @brief Housekeeping operations
 *
 * In this module all the housekeeping operations related to GVT computation phase
 * are present.
 *
 * SPDX-FileCopyrightText: 2008-2025 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <core/core.h>
#include <lp/lp.h>

/**
 * @brief Check whether a LP needs to perform fossil collection
 * @param lp the Logical Process to check
 * @return true if the LP needs to perform fossil collection else false
 */
#define fossil_is_needed(lp) ((lp)->fossil_epoch != fossil_epoch_current)

/// The current fossil collection epoch
extern __thread unsigned fossil_epoch_current;

extern void fossil_on_gvt(simtime_t current_gvt);
extern void fossil_lp_collect(struct lp_ctx *lp);
