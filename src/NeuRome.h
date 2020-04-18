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

extern void ScheduleNewEvent(unsigned receiver, simtime_t timestamp,
	unsigned event_type, const void *event_content, unsigned event_size);
extern void SetState(void *new_state);

extern double Random(void);
extern uint64_t RandomU64(void);
extern double Expent(double mean);

enum _topology_geometry_t {
	TOPOLOGY_HEXAGON,	//!< hexagonal grid topology
	TOPOLOGY_SQUARE,	//!< square grid topology
	TOPOLOGY_RING,		//!< a ring shaped topology walkable in a single direction
	TOPOLOGY_BIDRING,	//!< a ring shaped topology
	TOPOLOGY_TORUS,		//!< a torus shaped grid topology (a wrapping around square topology)
	TOPOLOGY_STAR,		//!< a star shaped topology
	TOPOLOGY_MESH,		//!< an arbitrary shaped topology
};

typedef enum _direction_t {
	DIRECTION_N,	//!< North direction
	DIRECTION_S,	//!< South direction
	DIRECTION_E,	//!< East direction
	DIRECTION_W,	//!< West direction
	DIRECTION_NE,	//!< North-east direction
	DIRECTION_SW,	//!< South-west direction
	DIRECTION_NW,	//!< North-west direction
	DIRECTION_SE,	//!< South-east direction

	DIRECTION_INVALID = INT_MAX	//!< A generic invalid direction
} direction_t;

__attribute((weak)) extern struct _topology_settings_t {
	const enum _topology_geometry_t default_geometry;	//!< The default geometry to use when nothing else is specified
	const unsigned out_of_topology;				//!< The minimum number of LPs needed out of the topology
} topology_settings;

__attribute__ ((pure)) unsigned RegionsCount(void);
__attribute__ ((pure)) unsigned DirectionsCount(void);
__attribute__ ((pure)) unsigned GetReceiver(unsigned from, direction_t to);
unsigned FindReceiver(void);
