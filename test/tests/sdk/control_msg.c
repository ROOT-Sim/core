#include <test.h>

#include <ROOT-Sim/sdk.h>
#include <distributed/control_msg.h>
#include <distributed/mpi.h>

#include <assert.h>
#include <stdbool.h>

#include "control_msg.h"

static long value = 1234L;

static unsigned ctrl_msg_id1 = 0;
static unsigned ctrl_msg_id2 = 0;

static unsigned short inv1 = 0;
static unsigned short inv2 = 0;

void handler(unsigned ctrl_msg_id, const void *payload)
{
	assert(*(long *)payload == value);

	if(ctrl_msg_id == ctrl_msg_id1) {
		inv1++;
	} else if(ctrl_msg_id == ctrl_msg_id2) {
		inv2++;
	} else {
		assert(false);
	}
}

int test_ctrl_msg(bool distributed)
{
	ctrl_msg_id1 = control_msg_register_handler(handler);
	ctrl_msg_id2 = control_msg_register_handler(handler);

	assert(ctrl_msg_id1 == MSG_CTRL_DEFAULT_END);
	assert(ctrl_msg_id2 > MSG_CTRL_DEFAULT_END);

	control_msg_broadcast(ctrl_msg_id1, &value, sizeof(value));
	control_msg_broadcast(ctrl_msg_id2, &value, sizeof(value));

	if(distributed) {
		mpi_remote_msg_handle();
	}

	assert(inv1 != 0);
	assert(inv2 != 0);

	return inv1 != inv2;
}

void ProcessEvent(_unused lp_id_t me, _unused simtime_t now, _unused unsigned event_type, _unused const void *content,
    _unused unsigned size, _unused void *s)
{}
