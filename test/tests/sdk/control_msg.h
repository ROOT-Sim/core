/**
* @file test/tests/sdk/control_msg.h
*
* @brief Test: Higher-level control messages
*
* SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
* SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <test.h>

extern int test_ctrl_msg(bool distributed);
extern void ProcessEvent(_unused lp_id_t me, _unused simtime_t now, _unused unsigned event_type,
    _unused const void *content, _unused unsigned size, _unused void *s);
