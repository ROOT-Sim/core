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
#define msg_bare_size(msg) (offsetof(struct lp_msg, pl) + (msg)->pl_size)

#define msg_id_get(msg, cur_phase) 					\
	((((uintptr_t)msg) ^ ((nid + 1) << 2)) | ((cur_phase) << 1))
#define msg_id_phase_get(msg_id) ((msg_id) & 1U)
#define msg_id_phase_set(msg_id, phase) 				\
__extension__({								\
	(msg_id) &= ~((uintptr_t) 1U);					\
	(msg_id) += (phase);						\
})

/// A model simulation message
struct lp_msg {
	/// The id of the recipient LP
	lp_id_t dest;
	/// The intended destination logical time of this message
	simtime_t dest_t;
	/// The message type, a user controlled field
	uint_fast32_t m_type;
	 /// The message payload size
	uint_fast32_t pl_size;
	union {
		/// The flags to handle local anti messages
		atomic_int flags;
		/// The message unique id, used for inter-node anti messages
		uintptr_t msg_id;
	};
	/// The initial part of the payload
	unsigned char pl[BASE_PAYLOAD_SIZE];
	/// The continuation of the payload
	unsigned char extra_pl[];
};

enum msg_flag {
	MSG_FLAG_ANTI 		= 1,
	MSG_FLAG_PROCESSED	= 2
};



