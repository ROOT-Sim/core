/**
* @file lib/topology/topology.h
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
#pragma once

#include <core/core.h>

#include <limits.h>
#include <stdint.h>

extern void topology_global_init(void);

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

	// FIXME this is bad if the n_lps is more than INT_MAX - 1
	DIRECTION_INVALID = INT_MAX	//!< A generic invalid direction
};

/// This is declared by the model to setup the topology module
extern struct topology_settings_t {
	/// The default geometry to use when nothing else is specified
	enum _topology_geometry_t default_geometry;
	/// The minimum number of LPs needed for out of the topology logic
	lp_id_t out_of_topology;
} topology_settings;

extern __attribute__ ((pure)) lp_id_t RegionsCount(void);
extern __attribute__ ((pure)) lp_id_t DirectionsCount(void);
extern __attribute__ ((pure)) lp_id_t GetReceiver(lp_id_t from, enum _direction_t direction);
extern lp_id_t FindReceiver(void);
