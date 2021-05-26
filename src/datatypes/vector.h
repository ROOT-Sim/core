/**
 * @file datatypes/vector.h
 *
 * @brief Dynamic array datatype of buffers of a given size
 *
 * Dynamic array datatype of buffers of a given size.
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once
#include <stdlib.h>
#include <stdbool.h>

struct vector;

extern void *init_vector(size_t nmemb, size_t size);
extern bool insert_at(struct vector *arr, size_t pos, void *el);
extern bool push_back(struct vector *arr, void *el);
extern bool read_from(struct vector *arr, size_t pos, void *buffer);
extern bool extract_from(struct vector *arr, size_t pos, void *buffer);
extern void free_vector(struct vector *arr);
extern void *freeze_vector(struct vector *arr);
