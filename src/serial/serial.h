/**
 * @file serial/serial.h
 *
 * @brief Sequential simulation engine
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <core/core.h>

extern void serial_model_init(void);

extern void serial_simulation(void);

visible extern void ScheduleNewEvent(lp_id_t receiver, simtime_t timestamp,
	unsigned event_type, const void *payload, unsigned payload_size);

visible __attribute__ ((pure)) extern lp_id_t lp_id_get(void);
visible __attribute__ ((pure)) extern struct lib_ctx *lib_ctx_get(void);
