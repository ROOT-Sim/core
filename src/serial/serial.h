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

extern int serial_simulation(void);
