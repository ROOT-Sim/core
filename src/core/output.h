/**
 * @file coure/output.h
 *
 * @brief Committed output management functions
 *
 * Committed output management functions
 *
 * SPDX-FileCopyrightText: 2008-2023 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <datatypes/array.h>

struct output_data {
	unsigned type;
	void *content;
	unsigned size;
};

typedef dyn_array(struct output_data) output_array_t;

/**
 * @brief Free the outputs stored for later from a message
 * @param output_array the output_data from the message
 */
void free_msg_outputs(output_array_t *output_array);
