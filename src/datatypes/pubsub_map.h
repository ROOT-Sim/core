/**
 * @file pubsub_map.h
 *
 * @brief Pubsub msgs map header
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <stdint.h>

#include <core/core.h>
#include <datatypes/array.h>

#define MAX_LOAD_FACTOR 0.85
#define MIN_LOAD_FACTOR 0.05

typedef uint32_t pubsub_map_hash_nt;

struct pubsub_map {
	struct pubsub_map_node {
		pubsub_map_hash_nt hash;
		struct lp_msg *msg;
	} *n;
	array_count_t capacity_mo;
	array_count_t count;
};

extern void pubsub_map_init(struct pubsub_map *m);
extern void pubsub_map_fini(struct pubsub_map *m);
extern array_count_t pubsub_map_count(const struct pubsub_map *m);
extern struct lp_msg *pubsub_map_add(struct pubsub_map *m, struct lp_msg *msg);
extern void pubsub_map_fossil_collect(struct pubsub_map *m, simtime_t gvt);
