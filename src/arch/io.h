/**
 * @file arch/io.h
 *
 * @brief Generic input-output facilities
 *
 * This header declares architecture independent input-oputput facilities for
 * the use in the simulator
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <arch/platform.h>

#include <stdbool.h>
#include <stdio.h>

#define IO_TIME_BUFFER_LEN 26

extern bool io_terminal_can_colorize(void);
extern void io_local_time_get(char res[IO_TIME_BUFFER_LEN]);
extern FILE *io_file_tmp_get(void);
