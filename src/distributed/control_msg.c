/**
 * @file distributed/control_msg.c
 *
 * @brief MPI remote control messages module
 *
 * The module in which remote control messages are wired to the other modules
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <ROOT-Sim/sdk.h>
#include <distributed/control_msg.h>
#include <distributed/mpi.h>

struct library_handler {
	unsigned control_msg_id;
	control_msg_handler_t handler;
};

static struct library_handler *library_handlers = NULL;
static size_t library_handlers_capacity = 0;
static size_t library_handlers_size = 0;
static int next_control_msg_id = MSG_CTRL_DEFAULT_END;

int control_msg_register_handler(control_msg_handler_t handler)
{
	int ret = next_control_msg_id;

	if(unlikely(library_handlers == NULL)) {
		library_handlers_capacity = INITIAL_HANDLERS_CAPACITY;
		library_handlers = malloc(library_handlers_capacity * sizeof(*library_handlers));
	}

	if(library_handlers_size == library_handlers_capacity) {
		library_handlers_capacity *= 2;
		library_handlers = realloc(library_handlers, library_handlers_capacity * sizeof(*library_handlers));
	}
	library_handlers[library_handlers_size].control_msg_id = next_control_msg_id++;
	library_handlers[library_handlers_size++].handler = handler;

	return ret;
}

/**
 * @brief Invoke a library handler
 * @param code the control message ID
 * @param payload the payload of the control message
 */
void invoke_library_handler(unsigned code, const void *payload)
{
	for(size_t i = 0; i < library_handlers_size; i++) {
		if(library_handlers[i].control_msg_id == code) {
			library_handlers[i].handler(code, payload);
			return;
		}
	}
}

/**
 * @brief Handle a received control message
 * @param ctrl the tag of the received control message
 */
void control_msg_process(unsigned ctrl, void *payload)
{
	switch(ctrl) {
		case MSG_CTRL_GVT_START:
			gvt_start_processing();
			break;
		case MSG_CTRL_GVT_DONE:
			gvt_on_done_ctrl_msg();
			break;
		case MSG_CTRL_TERMINATION:
			termination_on_ctrl_msg();
			break;
		default:
			invoke_library_handler(ctrl, payload);
	}
}

void control_msg_broadcast(unsigned ctrl, const void *payload, size_t size)
{
	if(size > CONTROL_MSG_PAYLOAD_SIZE) {
		logger(LOG_FATAL, "The payload of the control message is too large");
		abort();
	}
	if(global_config.serial || n_nodes == 1) {
		invoke_library_handler(ctrl, payload);
	} else {
		mpi_library_control_msg_broadcast(ctrl, payload, size);
	}
}
