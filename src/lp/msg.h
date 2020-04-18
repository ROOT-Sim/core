#pragma once

#include <core/core.h>
#include <stdatomic.h>
#include <stddef.h>

#define BASE_PAYLOAD_SIZE 48

#define msg_is_before(msg_a, msg_b) ((msg_a)->dest_t < (msg_b)->dest_t)

#define msg_bare_size(msg) (offsetof(lp_msg, pl) + (msg)->pl_size)

struct _lp_msg {
	lp_id_t dest;
	simtime_t dest_t;
#ifndef NEUROME_SERIAL
	atomic_uint flags;
#endif
	uint_fast32_t m_type;
	uint_fast32_t pl_size;
	unsigned char pl[BASE_PAYLOAD_SIZE];
	unsigned char extra_pl[];
};

typedef struct _lp_msg lp_msg;

#ifndef NEUROME_SERIAL
extern __thread lp_msg *current_msg;
#endif

enum _msg_flag{
	MSG_FLAG_ANTI 		= 1U,
	MSG_FLAG_PROCESSED	= 2U,
	MSG_FLAG_REMOTE		= 4U
};



