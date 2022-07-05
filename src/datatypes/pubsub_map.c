/**
 * @file pubsub_map.c
 *
 * @brief Pubsub msgs map
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <datatypes/pubsub_map.h>

#include <mm/msg_allocator.h>
#include <modules/publish_subscribe/pubsub.h>

#include <memory.h>
#include <stdint.h>
#include <stdio.h>

// must be a power of two
#define MAP_INITIAL_CAPACITY 16
#define DIB(curr_i, hash, capacity_mo) 	(((curr_i) - (hash)) & (capacity_mo))
#define SWAP_VALUES(a, b) 						\
	do {								\
		__typeof(a) _tmp = (a);					\
		(a) = (b);						\
		(b) = _tmp;						\
	} while(0)

// Adapted from http://xorshift.di.unimi.it/splitmix64.c PRNG,
// written by Sebastiano Vigna (vigna@acm.org)
// TODO benchmark further and select a possibly better hash function
static inline pubsub_map_hash_nt get_hash(const struct lp_msg *msg)
{
	uint64_t z = 0x9e3779b97f4a7c15 +
			(((uint64_t)msg->m_seq) << 32) + (msg->raw_flags >> MSG_FLAGS_BITS);
	z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
	z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
	return (pubsub_map_hash_nt)((z ^ (z >> 31)) >> 32);
}

void pubsub_map_init(struct pubsub_map *m)
{
	// this is the effective capacity_minus_one
	// this trick saves us some subtractions when we
	// use the capacity as a bitmask to
	// select the relevant hash bits for table indexing
	m->capacity_mo = MAP_INITIAL_CAPACITY - 1;
	m->count = 0;
	m->n = mm_alloc(sizeof(*m->n) * MAP_INITIAL_CAPACITY);
	memset(m->n, 0, sizeof(*m->n) * MAP_INITIAL_CAPACITY);
}

void pubsub_map_fini(struct pubsub_map *m)
{
	struct pubsub_map_node *rmv = m->n;

	for (array_count_t i = 0, j = 0; j < m->count; ++i, ++j) {
		while (rmv[i].msg == NULL)
			++i;

		if(rmv[i].msg->raw_flags & MSG_FLAG_ANTI) {
			msg_allocator_free(rmv[i].msg);
		} else {
			pubsub_thread_msg_free(rmv[i].msg);
		}
	}

	mm_free(m->n);
}

array_count_t pubsub_map_count(const struct pubsub_map *n)
{
	return n->count;
}

static struct lp_msg *pubsub_map_insert_hashed(struct pubsub_map *m,
		struct pubsub_map_node ins)
{
	struct pubsub_map_node *nodes = m->n;
	const array_count_t capacity_mo = m->capacity_mo;
	array_count_t i = ins.hash & capacity_mo;
	// distance from initial bucket
	array_count_t dib = 0;

	// linear probing with robin hood hashing
	// https://cs.uwaterloo.ca/research/tr/1986/CS-86-14.pdf by Pedro Celis
	while (1) {
		if (nodes[i].msg == NULL) {
			nodes[i] = ins;
			return NULL;
		}

		if (nodes[i].hash == ins.hash &&
				(nodes[i].msg->raw_flags >> MSG_FLAGS_BITS) == (ins.msg->raw_flags >> MSG_FLAGS_BITS) &&
				nodes[i].msg->m_seq == ins.msg->m_seq)
			return nodes[i].msg;

		i = (i + 1) & capacity_mo;
		++dib;

		array_count_t n_dib = DIB(i, nodes[i].hash, capacity_mo);
		if (dib > n_dib) {
			// found a "richer" cell:
			// swap the pairs and continue looking for a hole
			dib = n_dib;
			SWAP_VALUES(nodes[i], ins);
		}
	}
}

static void pubsub_map_expand(struct pubsub_map *m)
{
	if(MAX_LOAD_FACTOR * m->capacity_mo >= m->count)
		return;

	m->capacity_mo = 2 * m->capacity_mo + 1;

	struct pubsub_map_node *rmv = m->n;
	m->n = mm_alloc(sizeof(*m->n) * (m->capacity_mo + 1));
	memset(m->n, 0, sizeof(*m->n) * (m->capacity_mo + 1));

	for (array_count_t i = 0, j = 0; j < m->count; ++i, ++j) {
		while (rmv[i].msg == NULL)
			++i;

		pubsub_map_insert_hashed(m, rmv[i]);
	}
	mm_free(rmv);
}

struct lp_msg *pubsub_map_add(struct pubsub_map *m, struct lp_msg *msg)
{
	struct pubsub_map_node ins = {
		.hash = get_hash(msg),
		.msg = msg
	};

	struct lp_msg *ret = pubsub_map_insert_hashed(m, ins);
	if (ret == NULL) {
		++m->count;
		pubsub_map_expand(m);
	}
	return ret;
}

void pubsub_map_fossil_collect(struct pubsub_map *m, simtime_t gvt)
{
	struct pubsub_map_node *rmv = m->n;
	m->n = mm_alloc(sizeof(*m->n) * (m->capacity_mo + 1));
	memset(m->n, 0, sizeof(*m->n) * (m->capacity_mo + 1));

	array_count_t old_c = m->count;
	for (array_count_t i = 0, j = 0; j < old_c; ++i, ++j) {
		while (rmv[i].msg == NULL)
			++i;
		if (rmv[i].msg->dest_t < gvt) {
			if(rmv[i].msg->raw_flags & MSG_FLAG_ANTI) {
				msg_allocator_free(rmv[i].msg);
			} else {
				pubsub_thread_msg_free(rmv[i].msg);
			}
			--m->count;
		} else
			pubsub_map_insert_hashed(m, rmv[i]);
	}
	mm_free(rmv);
}
