/**
* @file lib/topology/topology.c
*
* @brief Topology library
*
* This library is allows models to setup and query different topologies.
*
* @copyright
* Copyright (C) 2008-2021 HPDCS Group
* https://hpdcs.github.io
*
* This file is part of ROOT-Sim (ROme OpTimistic Simulator).
*
* ROOT-Sim is free software; you can redistribute it and/or modify it under the
* terms of the GNU General Public License as published by the Free Software
* Foundation; only version 3 of the License applies.
*
* ROOT-Sim is distributed in the hope that it will be useful, but WITHOUT ANY
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
* A PARTICULAR PURPOSE. See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with
* ROOT-Sim; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include <lib/topology/topology.h>

#include <core/intrinsics.h>
#include <lib/lib_internal.h>

#include <math.h>
#include <memory.h>

__attribute((weak)) struct topology_settings_t topology_settings;

/// this is used to store the common characteristics of the topology
struct {
	lp_id_t regions_cnt; 			/**< the number of LPs involved in the topology */
	uint32_t edge; 				/**< the pre-computed edge length (if it makes sense for the current topology geometry) */
	enum _topology_geometry_t geometry;	/**< the topology geometry (see ROOT-Sim.h) */
} topology_global;

/**
 * Initialize the topology module for each LP hosted on the machine.
 * This needs to be called right after LP basic initialization before starting to process events.
 */
void topology_global_init(void)
{
	if (!topology_settings.default_geometry &&
		!topology_settings.out_of_topology)
		// the strong symbol isn't defined: we aren't needed
		return;

	// set default values
	const lp_id_t regions_cnt = n_lps - topology_settings.out_of_topology;
	topology_global.regions_cnt = regions_cnt;
	topology_global.geometry = topology_settings.default_geometry;
	// compute the edge value for topologies it makes sense for
	unsigned edge;

	switch (topology_global.geometry) {
		case TOPOLOGY_SQUARE:
		case TOPOLOGY_HEXAGON:
		case TOPOLOGY_TORUS:
			edge = sqrt(regions_cnt);
			// we make sure there are no "lonely" LPs
			if (edge * edge != regions_cnt) {
				log_log(LOG_FATAL, "Invalid number of regions for this topology geometry (must be a square number)\n");
				exit(-1);
			}
			break;
		default:
			// the edge value is actually unused
			edge = 0;
			break;
	}
	// set the edge value
	topology_global.edge = edge;
}

__attribute__ ((pure)) lp_id_t RegionsCount(void)
{
	return topology_global.regions_cnt;
}

__attribute__ ((pure)) lp_id_t DirectionsCount(void)
{
	switch (topology_global.geometry) {
	case TOPOLOGY_MESH:
		return topology_global.regions_cnt - 1;
	case TOPOLOGY_HEXAGON:
		return 6;
	case TOPOLOGY_TORUS:
	case TOPOLOGY_SQUARE:
		return 4;
	case TOPOLOGY_STAR:
		return 2;
	case TOPOLOGY_RING:
		return 1;
	case TOPOLOGY_BIDRING:
		return 2;
	}
	return UINT_MAX;
}

__attribute__ ((pure)) lp_id_t GetReceiver(lp_id_t from,
					    enum _direction_t direction)
{
	const lp_id_t sender = from;
	const uint32_t edge = topology_global.edge;
	const lp_id_t regions_cnt = topology_global.regions_cnt;
	unsigned x, y;

	if (unlikely(regions_cnt <= from))
		return DIRECTION_INVALID;

	switch (topology_global.geometry) {

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
		return likely((lp_id_t)direction < regions_cnt) ? direction : DIRECTION_INVALID;

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
		return likely(direction == DIRECTION_N) ? direction : DIRECTION_INVALID;

	case TOPOLOGY_STAR:
		if(sender) {
			if(!direction)
				return 0;
		} else {
			if((uint64_t)direction + 1 < regions_cnt)
				return direction + 1;
		}
	}
	return DIRECTION_INVALID;
}

lp_id_t FindReceiver(void)
{
	const lp_id_t dir_cnt = DirectionsCount();
	const unsigned bits = 64 - intrinsics_clz(dir_cnt);
	uint64_t rnd = RandomU64();
	unsigned i = 64;
	do {
		lp_id_t dir = rnd & ((UINT64_C(1) << bits) - 1);
		if (dir < dir_cnt) {
			dir += 2 * (topology_global.geometry == TOPOLOGY_HEXAGON);
			const lp_id_t ret = GetReceiver(lp_id_get(), dir);
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
