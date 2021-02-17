/**
 * @file datatypes/remote_msg_map.h
 *
 * @brief Message map datatype
 *
 * Message map datatype
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <lp/msg.h>

extern void remote_msg_map_global_init(void);
extern void remote_msg_map_global_fini(void);
extern void remote_msg_map_fossil_collect(simtime_t current_gvt);
extern void remote_msg_map_match(uintptr_t msg_id, nid_t nid, struct lp_msg *msg);
