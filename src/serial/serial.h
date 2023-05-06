/**
 * @file serial/serial.h
 *
 * @brief Sequential simulation engine
 *
 * SPDX-FileCopyrightText: 2008-2023 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <core/core.h>

extern void serial_simulation_init(void);
extern int serial_simulation(void);
extern void ScheduleNewEvent_serial(lp_id_t receiver, simtime_t timestamp, unsigned event_type, const void *payload,
									unsigned payload_size);
