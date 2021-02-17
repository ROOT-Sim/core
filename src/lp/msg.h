/**
 * @file lp/msg.h
 *
 * @brief Message management functions
 *
 * Message management functions
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <core/core.h>

#include <stdatomic.h>
#include <stddef.h>

#define BASE_PAYLOAD_SIZE 48

#define msg_is_before(msg_a, msg_b) ((msg_a)->dest_t < (msg_b)->dest_t)
#define msg_bare_size(msg) (offsetof(struct lp_msg, pl) + (msg)->pl_size)

#define msg_id_get(msg, cur_phase) 					\
	(((uintptr_t)msg) | ((unsigned)(cur_phase) << 1))
#define msg_id_phase_get(msg_id) (((msg_id) >> 1) & 1U)
#define msg_id_anti_phase_get(msg_id) ((msg_id) & 1U)
#define msg_id_anti_phase_set(msg_id, phase) 				\
__extension__({								\
	(msg_id) &= ~((uintptr_t) 1U);					\
	(msg_id) |= (phase);						\
})

/// A model simulation message
struct lp_msg {
	/// The id of the recipient LP
	lp_id_t dest;
	/// The intended destination logical time of this message
	simtime_t dest_t;
	/// The message type, a user controlled field
	uint_fast32_t m_type;
#if LOG_LEVEL <= LOG_DEBUG
	lp_id_t send;
	simtime_t send_t;
#endif
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



