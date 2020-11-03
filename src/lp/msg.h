/**
 * @file lp/msg.h
 *
 * @brief Message management functions
 *
 * Message management functions
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
#include <stdatomic.h>
#include <stddef.h>

#define BASE_PAYLOAD_SIZE 48

#define msg_is_before(msg_a, msg_b) ((msg_a)->dest_t < (msg_b)->dest_t)
#define msg_bare_size(msg) (offsetof(lp_msg, pl) + (msg)->pl_size)

#ifdef NEUROME_MPI
#define msg_id_get(msg, cur_phase) 					\
	((((uintptr_t)msg) ^ ((nid + 1) << 2)) | ((cur_phase) << 1))
#define msg_id_phase_get(msg_id) ((msg_id) & 1U)
#define msg_id_phase_set(msg_id, phase) 				\
__extension__({								\
	(msg_id) &= ~((uintptr_t) 1U);					\
	(msg_id) += (phase);						\
})

#endif

struct _lp_msg {
	lp_id_t dest;
	simtime_t dest_t;
	uint_fast32_t m_type;
	uint_fast32_t pl_size;
#ifndef NEUROME_SERIAL
#ifdef NEUROME_MPI
	union {
#endif
		atomic_int flags;
#ifdef NEUROME_MPI
		uintptr_t msg_id;
	};
#endif
#endif
	unsigned char pl[BASE_PAYLOAD_SIZE];
	unsigned char extra_pl[];
};

typedef struct _lp_msg lp_msg;

enum msg_flag_t {
	MSG_FLAG_ANTI 		= 1,
	MSG_FLAG_PROCESSED	= 2
};



