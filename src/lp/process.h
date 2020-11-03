/**
 * @file lp/process.h
 *
 * @brief LP state management functions
 *
 * LP state management functions
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

#include <datatypes/array.h>
#include <gvt/gvt.h>
#include <lp/msg.h>

struct process_data {
	dyn_array(lp_msg *) past_msgs;	//!< todo documentation
	dyn_array(lp_msg *) sent_msgs;	//!< todo documentation
};

extern void process_lp_init(void);
extern void process_lp_deinit(void);
extern void process_lp_fini(void);

extern void process_msg(void);
extern void process_msg_sent(lp_msg *msg);
