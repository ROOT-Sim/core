/**
 * @file lib/topology/topology.h
 *
 * @brief Topology library
 *
 * This library is allows models to setup and query different topologies.
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <core/core.h>

#include <limits.h>
#include <stdint.h>

enum _topology_geometry_t {
	TOPOLOGY_HEXAGON = 1,	//!< hexagonal grid topology
	TOPOLOGY_SQUARE,	//!< square grid topology
	TOPOLOGY_RING,		//!< a ring shaped topology walkable in a single direction
	TOPOLOGY_BIDRING,	//!< a ring shaped topology
	TOPOLOGY_TORUS,		//!< a torus shaped grid topology (a wrapping around square topology)
	TOPOLOGY_STAR,		//!< a star shaped topology
	TOPOLOGY_MESH,		//!< an arbitrary shaped topology
};

enum _direction_t {
	DIRECTION_N,	//!< North direction
	DIRECTION_S,	//!< South direction
	DIRECTION_E,	//!< East direction
	DIRECTION_W,	//!< West direction
	DIRECTION_NE,	//!< North-east direction
	DIRECTION_SW,	//!< South-west direction
	DIRECTION_NW,	//!< North-west direction
	DIRECTION_SE,	//!< South-east direction

	DIRECTION_INVALID = INT_MAX	//!< A generic invalid direction
};

struct topology_t;

extern struct topology_t *TopologyInit(enum _topology_geometry_t geometry, unsigned int out_of_topology);
extern unsigned long long RegionsCount(const struct topology_t *topology);
extern unsigned long long DirectionsCount(const struct topology_t *topology);
extern lp_id_t GetReceiver(const struct topology_t *topology, lp_id_t from, enum _direction_t direction);
extern lp_id_t FindReceiver(const struct topology_t *topology);
