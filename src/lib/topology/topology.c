/**
 * @file lib/topology/topology.c
 *
 * @brief Topology library
 *
 * This library is allows models to setup and query different topologies.
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <lib/topology/topology.h>

#include <core/intrinsics.h>
#include <lib/lib_internal.h>
#include <mm/mm.h>

#include <math.h>
#include <memory.h>

/// this is used to store the common characteristics of the topology
struct topology_t {
	enum _topology_geometry_t geometry; /**< The geometry associated with this configuration */
	uint64_t regions_cnt;      /**< the number of LPs involved in the topology */
	uint32_t edge;             /**< the pre-computed edge length (if it makes sense for the current topology geometry) */
};

/**
 * Initialize the topology module for each LP hosted on the machine.
 * This needs to be called right after LP basic initialization before starting to process events.
 *
 * @todo: out of topology should go, in favor of file-based configuration
 * @todo: the mm_alloc() call here shall be redirected to the LP allocator IF a mutable topology (e.g. graph) is used.
 */
struct topology_t *TopologyInit(enum _topology_geometry_t geometry, unsigned int out_of_topology)
{
	struct topology_t *topology = mm_alloc(sizeof(*topology));
	memset(topology, 0, sizeof(*topology));

	topology->geometry = geometry;

	// set default values
	if (unlikely(out_of_topology >= n_lps)) {
		log_log(LOG_FATAL, "Invalid number of regions for this topology geometry (out_of_topology too large)");
		exit(-1);
	}

	const lp_id_t regions_cnt = n_lps - out_of_topology;
	topology->regions_cnt = regions_cnt;

	// compute the edge value for topologies it makes sense for
	unsigned edge;
	switch (topology->geometry) {
		case TOPOLOGY_SQUARE:
		case TOPOLOGY_HEXAGON:
		case TOPOLOGY_TORUS:
			edge = sqrt(regions_cnt);
			// we make sure there are no "lonely" LPs
			if (edge * edge != regions_cnt) {
				log_log(LOG_FATAL,
					"Invalid number of regions for this topology geometry (must be a square number)");
				exit(-1);
			}
			break;
		case TOPOLOGY_MESH:
		case TOPOLOGY_STAR:
		case TOPOLOGY_RING:
		case TOPOLOGY_BIDRING:
			// the edge value is actually unused
			edge = 0;
			break;
		default:
			log_log(LOG_FATAL, "Invalid topology geometry");
			exit(-1);
	}
	// set the edge value
	topology->edge = edge;

	return topology;
}

unsigned long long RegionsCount(const struct topology_t *topology)
{
	return topology->regions_cnt;
}

unsigned long long DirectionsCount(const struct topology_t *topology)
{
	switch (topology->geometry) {
		case TOPOLOGY_MESH:
			return topology->regions_cnt;
		case TOPOLOGY_HEXAGON:
			return 6;
		case TOPOLOGY_TORUS:
		case TOPOLOGY_SQUARE:
			return 4;
		case TOPOLOGY_STAR:
			return 1;
		case TOPOLOGY_RING:
			return 1;
		case TOPOLOGY_BIDRING:
			return 2;
	}
	__builtin_unreachable();
}

lp_id_t GetReceiver(const struct topology_t *topology, lp_id_t from, enum _direction_t direction)
{
	const lp_id_t sender = from;
	const uint32_t edge = topology->edge;
	const uint64_t regions_cnt = topology->regions_cnt;
	unsigned x, y;

	if (unlikely(from >= regions_cnt))
		return DIRECTION_INVALID;

	switch (topology->geometry) {

		case TOPOLOGY_HEXAGON:
			y = sender / edge;
			x = sender - y * edge;

			switch (direction) {
				case DIRECTION_NW:
					x += (y & 1U) - 1;
					y -= 1;
					break;
				case DIRECTION_NE:
					x += (y & 1U);
					y -= 1;
					break;
				case DIRECTION_SW:
					x += (y & 1U) - 1;
					y += 1;
					break;
				case DIRECTION_SE:
					x += (y & 1U);
					y += 1;
					break;
				case DIRECTION_E:
					x += 1;
					break;
				case DIRECTION_W:
					x -= 1;
					break;
				default:
					return DIRECTION_INVALID;
			}
			return (x < edge && y < edge) ? y * edge + x : DIRECTION_INVALID;

		case TOPOLOGY_SQUARE:
			y = sender / edge;
			x = sender - y * edge;

			switch (direction) {
				case DIRECTION_N:
					y -= 1;
					break;
				case DIRECTION_S:
					y += 1;
					break;
				case DIRECTION_E:
					x += 1;
					break;
				case DIRECTION_W:
					x -= 1;
					break;
				default:
					return DIRECTION_INVALID;
			}
			return (x < edge && y < edge) ? y * edge + x : DIRECTION_INVALID;

		case TOPOLOGY_TORUS:
			y = sender / edge;
			x = sender - y * edge;

			switch (direction) {
				case DIRECTION_N:
					y += edge - 1;
					y %= edge;
					break;
				case DIRECTION_S:
					y += 1;
					y %= edge;
					break;
				case DIRECTION_E:
					x += 1;
					x %= edge;
					break;
				case DIRECTION_W:
					x += edge - 1;
					x %= edge;
					break;
				default:
					return DIRECTION_INVALID;
			}
			return y * edge + x;

		case TOPOLOGY_MESH:
			return likely((lp_id_t) direction < regions_cnt) ? direction : DIRECTION_INVALID;

		case TOPOLOGY_BIDRING:
			switch (direction) {
				case DIRECTION_N:
					return (sender + 1) % regions_cnt;
				case DIRECTION_S:
					return (sender + regions_cnt - 1) % regions_cnt;
				default:
					return DIRECTION_INVALID;
			}
		case TOPOLOGY_RING:
			return (sender + 1) % regions_cnt;

		case TOPOLOGY_STAR:
			if (sender)
				return 0;

			uint64_t dest;
			do {
				dest = Random() * regions_cnt;
			} while (dest != sender);
			return dest;
	}
	return DIRECTION_INVALID;
}

lp_id_t FindReceiver(const struct topology_t *topology)
{
	// Consider the degenerate case of a run with one single LP
	if (unlikely(topology->regions_cnt == 1))
		return 0;

	const lp_id_t dir_cnt = DirectionsCount(topology);
	const unsigned bits = 64 - intrinsics_clz(dir_cnt);
	uint64_t rnd = RandomU64();
	unsigned i = 64;
	do {
		lp_id_t dir = rnd & ((UINT64_C(1) << bits) - 1);
		if (dir < dir_cnt) {
			dir += 2 * (topology->geometry == TOPOLOGY_HEXAGON);
			const lp_id_t ret = GetReceiver(topology, lp_id_get(), dir);
			if (ret != DIRECTION_INVALID)
				return ret;
		}

		if (likely((i -= bits) >= bits)) {
			rnd >>= bits;
		} else {
			rnd = RandomU64();
			i = 64;
		}
	} while(1);
}
