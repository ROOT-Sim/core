/**
 * @file ROOT-Sim.h
 *
 * @brief ROOT-Sim header for model development
 *
 * This header defines all the symbols which are needed to develop a model
 * to be simulated on top of ROOT-Sim.
 *
 * This header is the only file which should be included when developing
 * a simulation model. All function prototypes exposed to the application
 * developer are exposed and defined here.
 *
 * @copyright
 * Copyright (C) 2008-2020 HPDCS Group
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

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define INIT	0
#define DEINIT	UINT_MAX

typedef double simtime_t;
typedef uint64_t lp_id_t;

/* A description of a particular option. */
struct ap_option {
	const char *name; /*!< The long option name. */
	int key; /*!< What key is returned for this option. */
	const char *arg; /*!< If non-NULL, this is the name of the argument
	associated with this option, which is required if set. */
	const char *doc; /*!< The doc string for this option. */
};

__attribute((weak)) extern struct ap_option model_options[];
__attribute((weak)) extern void model_parse(int key, const char *arg);

enum
{
	AP_KEY_INIT = 1 << 14,
	AP_KEY_FINI
};

extern lp_id_t n_lps;

extern void ScheduleNewEvent(lp_id_t receiver, simtime_t timestamp,
	unsigned event_type, const void *event_content, unsigned event_size);

extern void SetState(void *new_state);

extern double Random(void);
extern uint64_t RandomU64(void);
extern double Expent(double mean);
extern double Normal(void);

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

__attribute((weak)) extern struct _topology_settings_t {
	enum _topology_geometry_t default_geometry;	//!< The default geometry to use when nothing else is specified
	unsigned out_of_topology;			//!< The minimum number of LPs needed out of the topology
} topology_settings;

extern __attribute__ ((pure)) lp_id_t RegionsCount(void);
extern __attribute__ ((pure)) lp_id_t DirectionsCount(void);
extern __attribute__ ((pure)) lp_id_t GetReceiver(lp_id_t from, enum _direction_t direction);
extern lp_id_t FindReceiver(void);


