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

void (*ScheduleNewEvent)(lp_id_t receiver, simtime_t timestamp, unsigned event_type, const void *payload, unsigned payload_size);
