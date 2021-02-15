/**
 * @file gvt/fossil.h
 *
 * @brief Housekeeping operations
 *
 * In this module all the housekeeping operations related to GVT computation phase
 * are present.
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <core/core.h>

extern void fossil_collect(simtime_t current_gvt);
