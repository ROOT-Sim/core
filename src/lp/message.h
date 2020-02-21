#pragma once

#include <core/core.h>

#define FIXED_EVENT_PAYLOAD 32

struct _lp_msg {
	lp_id_t destination;
	simtime_t destination_time;
	unsigned payload_size;
	unsigned char payload[FIXED_EVENT_PAYLOAD];
	unsigned char additional_payload[];
};

typedef struct _lp_msg lp_msg;


