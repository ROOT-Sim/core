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
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

typedef double simtime_t;
typedef uint64_t lp_id_t;

typedef void (*ProcessEvent_t)(lp_id_t me, simtime_t now, unsigned event_type, const void *event_content,
    unsigned event_size, void *st);
typedef bool (*CanEnd_t)(lp_id_t me, const void *snapshot);

enum rootsim_event { MODEL_INIT = 65532, LP_INIT, LP_FINI, MODEL_FINI };

extern void (*ScheduleNewEvent)(lp_id_t receiver, simtime_t timestamp, unsigned event_type, const void *event_content,
    unsigned event_size);
extern void SetState(void *new_state);

extern void *rs_malloc(size_t req_size);
extern void *rs_calloc(size_t nmemb, size_t size);
extern void rs_free(void *ptr);
extern void *rs_realloc(void *ptr, size_t req_size);

extern double Random(void);
extern uint64_t RandomU64(void);
extern double Expent(double mean);
extern double Normal(void);
extern int RandomRange(int min, int max);
extern int RandomRangeNonUniform(int x, int min, int max);
extern double Gamma(unsigned ia);
extern double Poisson(void);
extern unsigned Zipf(double skew, unsigned limit);


enum log_level {
	LOG_SILENT, //!< Emit no message during the simulation
	LOG_TRACE,  //!< The logging level reserved to very low priority messages
	LOG_DEBUG,  //!< The logging level reserved to useful debug messages
	LOG_INFO,   //!< The logging level reserved to useful runtime messages
	LOG_WARN,   //!< The logging level reserved to unexpected, non deal breaking conditions
	LOG_ERROR,  //!< The logging level reserved to unexpected, problematic conditions
	LOG_FATAL   //!< The logging level reserved to unexpected, fatal conditions
};

enum _topology_geometry_t {
	TOPOLOGY_HEXAGON = 1, //!< hexagonal grid topology
	TOPOLOGY_SQUARE,      //!< square grid topology
	TOPOLOGY_RING,        //!< a ring shaped topology walkable in a single direction
	TOPOLOGY_BIDRING,     //!< a ring shaped topology
	TOPOLOGY_TORUS,       //!< a torus shaped grid topology (a wrapping around square topology)
	TOPOLOGY_STAR,        //!< a star shaped topology
	TOPOLOGY_MESH,        //!< an arbitrary shaped topology
};

enum _direction_t {
	DIRECTION_N,  //!< North direction
	DIRECTION_S,  //!< South direction
	DIRECTION_E,  //!< East direction
	DIRECTION_W,  //!< West direction
	DIRECTION_NE, //!< North-east direction
	DIRECTION_SW, //!< South-west direction
	DIRECTION_NW, //!< North-west direction
	DIRECTION_SE, //!< South-east direction

	// FIXME this is bad if the n_lps is more than INT_MAX - 1
	DIRECTION_INVALID = INT_MAX //!< A generic invalid direction
};

struct topology_t;

extern struct topology_t *TopologyInit(enum _topology_geometry_t geometry, unsigned int out_of_topology);
extern unsigned long long RegionsCount(const struct topology_t *topology);
extern unsigned long long DirectionsCount(const struct topology_t *topology);
extern lp_id_t GetReceiver(const struct topology_t *topology, lp_id_t from, enum _direction_t direction);
extern lp_id_t FindReceiver(const struct topology_t *topology);


/// A set of configurable values used by other modules
struct simulation_configuration {
	/// The number of LPs to be used in the simulation
	lp_id_t lps;
	/// The number of threads to be used in the simulation
	unsigned n_threads;
	/// The target termination logical time. Setting this value to zero means that LVT-based termination is disabled
	simtime_t termination_time;
	/// The gvt period expressed in microseconds
	unsigned gvt_period;
	/// The logger verbosity level
	enum log_level log_level;
	/// File where to write logged information: if not NULL, output is redirected to this file
	FILE *logfile;
	/// Path to the statistics file. If NULL, no statistics are produced.
	const char *stats_file;
	/// The checkpointing interval
	unsigned ckpt_interval;
	/// The seed used to initialize the pseudo random numbers, 0 for self-seeding
	uint64_t prng_seed;
	/// If set, worker threads are bound to physical cores
	bool core_binding;
	/// If set, the simulation will run on the serial runtime
	bool serial;
	/// Function pointer to the dispatching function
	ProcessEvent_t dispatcher;
	/// Function pointer to the termination detection function
	CanEnd_t committed;
};

extern int RootsimInit(struct simulation_configuration *conf);
extern int RootsimRun(void);
