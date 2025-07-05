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
#include <distributed/control_msg.h>
#include <lp/lp.h>

#include <stdatomic.h>

// TODO hide inside the module
#define TERMINATION_REQUESTED (-4.0)

extern void termination_global_init(void);
extern void termination_lp_init(struct lp_ctx *lp);
extern void termination_on_msg_process(struct lp_ctx *lp, simtime_t msg_time);
extern bool termination_on_gvt(simtime_t current_gvt);
extern void termination_on_lp_fossil(struct lp_ctx *lp, const simtime_t gvt_time);
extern void termination_on_lp_rollback(struct lp_ctx *lp, simtime_t msg_time);

extern void termination_on_lp_end_ctrl_msg(void);
extern void termination_on_orange_end_ctrl_msg(void);
extern void termination_on_purple_end_ctrl_msg(void);
