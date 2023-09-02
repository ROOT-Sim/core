/**
 * @file gvt/termination.h
 *
 * @brief Termination detection module
 *
 * SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <core/core.h>
#include <lp/lp.h>

#include <stdatomic.h>

extern bool termination_on_gvt(simtime_t current_gvt);
extern void termination_on_ctrl_msg(void);
