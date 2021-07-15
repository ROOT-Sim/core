/**
 * @file lib/state/state.h
 *
 * @brief LP main state management
 *
 * This library is responsible for allows LPs to set their state entry
 * point and change it at runtime.
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <core/visibility.h>

extern void state_lib_lp_init(void);

visible extern void SetState(void *state);
