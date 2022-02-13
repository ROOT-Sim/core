/**
 * @file core/core.c
 *
 * @brief Core ROOT-Sim functionalities
 *
 * Core ROOT-Sim functionalities
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <core/core.h>

__thread rid_t rid;
nid_t n_nodes = 1;
nid_t nid;

char *core_version = ROOTSIM_VERSION;

/**
 * @brief API to inject a new event in the simulation
 *
 * This is a function pointer that is setup at simulation startup to point to either
 * ScheduleNewEvent_parallel() in case of a parallel/distributed simulation, or to
 * ScheduleNewEvent_serial() in case of a serial simulation.
 *
 * @param receiver The ID of the LP that should receive the newly-injected message
 * @param timestamp The simulation time at which the event should be delivered at the recipient LP
 * @param event_type Numerical event type to be passed to the model's dispatcher
 * @param payload The event content
 * @param payload_size the size (in bytes) of the event content
 */
//__exported
void (*ScheduleNewEvent)(lp_id_t receiver, simtime_t timestamp, unsigned event_type, const void *payload, unsigned payload_size);
