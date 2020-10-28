#pragma once

#include <argp.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define INIT	0
#define DEINIT	UINT_MAX

typedef double simtime_t;
typedef uint64_t lp_id_t;

__attribute((weak)) extern struct argp model_argp;

extern lp_id_t n_lps;

extern void ScheduleNewEvent(lp_id_t receiver, simtime_t timestamp,
	unsigned event_type, const void *event_content, unsigned event_size);

extern void SetState(void *new_state);

extern double Random(void);
extern uint64_t RandomU64(void);
extern double Expent(double mean);
extern double Normal(void);

enum _topology_geometry_t {
	TOPOLOGY_HEXAGON,	//!< hexagonal grid topology
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
	const enum _topology_geometry_t default_geometry;	//!< The default geometry to use when nothing else is specified
	const unsigned out_of_topology;				//!< The minimum number of LPs needed out of the topology
} topology_settings;

extern __attribute__ ((pure)) lp_id_t RegionsCount(void);
extern __attribute__ ((pure)) lp_id_t DirectionsCount(void);
extern __attribute__ ((pure)) lp_id_t GetReceiver(lp_id_t from, enum _direction_t direction);
extern lp_id_t FindReceiver(void);

