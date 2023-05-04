/**
 * @file core/control_msg.c
 *
 * @brief MPI remote control messages module
 *
 * The module in which remote control messages are wired to the other modules
 *
 * SPDX-FileCopyrightText: 2008-2023 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include "include/ROOT-Sim/sdk.h"
#include "control_msg.h"
#include "distributed/mpi.h"

/// This structure is used to store the handlers for the control messages
struct library_handler {
	/// The control message id
	unsigned control_msg_id;
	/// The handler for the control message
	control_msg_handler_t handler;
};

/// The array of handlers for the control messages
static struct library_handler *library_handlers = NULL;
/// The capacity of the array of handlers for the control messages
static size_t library_handlers_capacity = 0;
/// The current number of used slots in the array of handlers for the control messages
static size_t library_handlers_size = 0;
/// The next control message ID
static int next_control_msg_id = FIRST_LIBRARY_CONTROL_MSG_ID;

/**
 * @brief Initialize the control message module
 */
void control_msg_init(void)
{
#ifndef NDEBUG
	if(library_handlers != NULL) {
		logger(LOG_WARN, "Trying to reinitialize control message module, ignoring!");
		return;
	}
#endif
	size_t size = INITIAL_HANDLERS_CAPACITY * sizeof(*library_handlers);
	void *ptr = malloc(size);
	if (ptr != NULL) {
		library_handlers_capacity = INITIAL_HANDLERS_CAPACITY;
		library_handlers = ptr;
	} else {
		logger(LOG_FATAL, "Error initializing control message module!");
		abort();
	}
}

/**
 * @brief Finalize the control message module
 */
void control_msg_fini(void)
{
	free(library_handlers);
}

int control_msg_register_handler(control_msg_handler_t handler)
{
	if (library_handlers_size >= library_handlers_capacity) {
		size_t new_capacity = library_handlers_capacity * 2;
		struct library_handler *new_handlers = realloc(library_handlers, new_capacity * sizeof(*new_handlers));
		if (new_handlers == NULL) {
			logger(LOG_FATAL, "Error registering external library handler!");
			abort();
		}
		library_handlers = new_handlers;
		library_handlers_capacity = new_capacity;
	}

	int ret = next_control_msg_id++;
	library_handlers[library_handlers_size].control_msg_id = ret;
	library_handlers[library_handlers_size].handler = handler;
	library_handlers_size++;
	return ret;
}


/**
 * @brief Invoke a library handler
 * @param code the control message ID
 * @param payload the payload of the control message
 */
void invoke_library_handler(unsigned code, const void *payload)
{
	for (size_t i = 0; i < library_handlers_size; i++) {
		if (library_handlers[i].control_msg_id == code) {
			library_handlers[i].handler(code, payload);
			return;
		}
	}
	logger(LOG_FATAL, "No library handler registered for code %u", code);
	abort();
}


/**
 * @brief Handle a received control message
 * @param ctrl the tag of the received control message
 */
void control_msg_process(enum platform_ctrl_msg_code ctrl)
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
#ifndef NDEBUG
			logger(LOG_WARN, "Unknown control message code %d", ctrl);
#else
			__builtin_unreachable();
#endif
	}
}

void control_msg_broadcast(unsigned ctrl, const void *payload, size_t size)
{
	if(global_config.serial || (n_nodes == 1)) {
		invoke_library_handler(ctrl, payload);
	} else {
		mpi_library_control_msg_broadcast(ctrl, payload, size);
	}
}
