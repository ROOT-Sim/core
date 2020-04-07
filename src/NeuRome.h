#pragma once

#include <argp.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define INIT	0
#define DEINIT	UINT_MAX

typedef double simtime_t;

__attribute((weak)) extern struct argp model_argp;

extern uint64_t n_lps;

extern void ScheduleNewEvent(unsigned receiver, simtime_t timestamp, unsigned event_type, const void *event_content, unsigned event_size);
extern void SetState(void *new_state);

enum _topology_geometry_t {
	TOPOLOGY_HEXAGON,	//!< hexagonal grid topology
	TOPOLOGY_SQUARE,	//!< square grid topology
	TOPOLOGY_RING,		//!< a ring shaped topology walkable in a single direction
	TOPOLOGY_BIDRING,	//!< a ring shaped topology
	TOPOLOGY_TORUS,		//!< a torus shaped grid topology (a wrapping around square topology)
	TOPOLOGY_STAR,		// this still needs to be properly implemented FIXME
	TOPOLOGY_MESH,		//!< an arbitrary shaped topology
};

typedef enum _direction_t {
	DIRECTION_E,	//!< DIRECTION_E
	DIRECTION_W,	//!< DIRECTION_W
	DIRECTION_N,	//!< DIRECTION_N
	DIRECTION_S,	//!< DIRECTION_S
	DIRECTION_NE,	//!< DIRECTION_NE
	DIRECTION_SW,	//!< DIRECTION_SW
	DIRECTION_NW,	//!< DIRECTION_NW
	DIRECTION_SE,	//!< DIRECTION_SE

	DIRECTION_INVALID = UINT_MAX	//!< A generic invalid direction
} direction_t;

__attribute((weak)) extern struct _topology_settings_t {
	const enum _topology_geometry_t default_geometry;	//!< The default geometry to use when nothing else is specified
	const unsigned out_of_topology;				//!< The minimum number of LPs needed out of the topology
} topology_settings;

unsigned GetReceiver(unsigned from, direction_t to);
