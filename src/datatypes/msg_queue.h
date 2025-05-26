/**
 * @file datatypes/msg_queue.h
 *
 * @brief Message queue datatype
 *
 * Message queue datatype
 *
 * SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <core/core.h>
#include <lp/msg.h>

extern void msg_queue_global_init(void);
extern void msg_queue_global_fini(void);
extern void msg_queue_init(void);
extern void msg_queue_fini(void);
extern struct lp_msg *msg_queue_extract(void);
extern void msg_queue_insert(struct lp_msg *msg);
extern void msg_queue_insert_self(struct lp_msg *msg);
