/**
* @file datatypes/msg_queue.h
*
* @brief Message queue datatype
*
* Message queue datatype
*
* @copyright
* Copyright (C) 2008-2020 HPDCS Group
* https://hpdcs.github.io
*
* This file is part of ROOT-Sim (ROme OpTimistic Simulator).
*
* ROOT-Sim is free software; you can redistribute it and/or modify it under the
* terms of the GNU General Public License as published by the Free Software
* Foundation; only version 3 of the License applies.
*
* ROOT-Sim is distributed in the hope that it will be useful, but WITHOUT ANY
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
* A PARTICULAR PURPOSE. See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with
* ROOT-Sim; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#pragma once

#include <core/core.h>
#include <lp/msg.h>

extern void msg_queue_global_init(void);
extern void msg_queue_global_fini(void);
extern void msg_queue_init(void);
extern void msg_queue_fini(void);
extern struct lp_msg *msg_queue_extract(void);
extern simtime_t msg_queue_time_peek(void);
extern void msg_queue_insert(struct lp_msg *msg);
