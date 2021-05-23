/**
 * @file gvt/termination.h
 *
 * @brief Termination detection module
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <core/core.h>

#include <stdatomic.h>

extern _Atomic(nid_t) nodes_to_end;

#define termination_cant_end() (atomic_load_explicit(&nodes_to_end, memory_order_relaxed) > 0)

extern void termination_global_init(void);
extern void termination_lp_init(void);
extern void termination_on_msg_process(simtime_t msg_time);
extern void termination_on_gvt(simtime_t current_gvt);
extern void termination_on_lp_rollback(simtime_t msg_time);
extern void termination_on_ctrl_msg(void);
extern void termination_force(void);

