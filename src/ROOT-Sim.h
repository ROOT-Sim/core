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
#include <float.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/// Simulation time data type
typedef double simtime_t;

/// The maximum value of the logical simulation time, semantically never
#define SIMTIME_MAX DBL_MAX

/// Logical Process ID data type
typedef uint64_t lp_id_t;

/**
 * @brief ProcessEvent callback function
 * @param me The logical process ID of the called LP
 * @param now The current simulation time
 * @param event_type The type of the simulation event
 * @param event_content The (model-specific) content of the simulation event
 * @param event_size The size of the event content
 * @param st The current state of the logical process
 *
 * This function is called by the simulation kernel whenever a new event is extracted from the event queue.
 * The event is executed in the speculative part of the simulation trajectory: any change to the simulation
 * state might be reverted by the simulation kernel in case a straggler event is detected.
 *
 * @warning The event content is not a copy, so it should not be modified. Failing to do so might lead to undefined
 * behavior in case of straggler events.
 * @warning Do not perform any non-rollbackable operation in this function (e.g., I/O).
 */
typedef void (*ProcessEvent_t)(lp_id_t me, simtime_t now, unsigned event_type, const void *event_content,
    unsigned event_size, void *st);

/**
 * @brief Determine if simulation can be halted.
 * @param me The logical process ID of the called LP
 * @param snapshot The committed state of the logical process
 *
 * @return true if the simulation can be halted, false otherwise
 *
 * This function receives a committed snapshot of the logical process state. It can be inspected to determine
 * whether the simulation can be halted, locally at the LP. The function should return true if the simulation
 * can be halted, false otherwise.
 *
 * @warning The snapshot is in the committed part of the simulation trajectory, so it should not be modified. Any
 * change to the snapshot might lead to undefined behavior.
 */
typedef bool (*CanEnd_t)(lp_id_t me, const void *snapshot);

enum rootsim_event {LP_INIT = 65534, LP_FINI};

/**
 * @brief API to inject a new event in the simulation
 *
 * This is a function pointer that is setup at simulation startup to point to either
 * ScheduleNewEvent_parallel() in case of a parallel/distributed simulation, or to
 * ScheduleNewEvent_serial() in case of a serial simulation.
 *
 * @param receiver The ID of the LP that should receive the newly-injected message
 * @param timestamp The simulation time at which the event should be delivered at the recipient LP
 * @param event_type Numerical event type to be passed to the model's dispatcher
 * @param event_content The event content
 * @param event_size The size (in bytes) of the event content
 */
extern void ScheduleNewEvent(lp_id_t receiver, simtime_t timestamp, unsigned event_type, const void *event_content,
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
	LOG_TRACE,  //!< The logging level reserved to very low priority messages
	LOG_DEBUG,  //!< The logging level reserved to useful debug messages
	LOG_INFO,   //!< The logging level reserved to useful runtime messages
	LOG_WARN,   //!< The logging level reserved to unexpected, non deal breaking conditions
	LOG_ERROR,  //!< The logging level reserved to unexpected, problematic conditions
	LOG_FATAL,   //!< The logging level reserved to unexpected, fatal conditions
	LOG_SILENT //!< Emit no message during the simulation
};

/********* TOPOLOGY LIBRARY ************/
struct topology;

enum topology_geometry {
	TOPOLOGY_HEXAGON = 1,	//!< hexagonal grid topology
	TOPOLOGY_SQUARE,	//!< square grid topology
	TOPOLOGY_TORUS,		//!< a torus shaped grid topology (a wrapping around square topology)
	TOPOLOGY_RING,		//!< a ring shaped topology walkable in a single direction
	TOPOLOGY_BIDRING,	//!< a ring shaped topology
	TOPOLOGY_STAR,		//!< a star shaped topology
	TOPOLOGY_FCMESH,	//!< a fully-connected mesh
	TOPOLOGY_GRAPH,		//!< a (weighted) graph topology
};

enum topology_direction {
	DIRECTION_E,	//!< East direction
	DIRECTION_W,	//!< West direction
	DIRECTION_N,	//!< North direction
	DIRECTION_S,	//!< South direction
	DIRECTION_NE,	//!< North-east direction
	DIRECTION_SW,	//!< South-west direction
	DIRECTION_NW,	//!< North-west direction
	DIRECTION_SE,	//!< South-east direction

	DIRECTION_RANDOM, //!< Get a random direction, depending on the topology
};

/// An invalid direction, used as error value for the functions which return a LP id
#define INVALID_DIRECTION UINT64_MAX

extern lp_id_t CountRegions(struct topology *topology);
extern lp_id_t CountDirections(lp_id_t from, struct topology *topology);
extern lp_id_t GetReceiver(lp_id_t from, struct topology *topology, enum topology_direction direction);

extern void ReleaseTopology(struct topology *topology);
extern bool AddTopologyLink(struct topology *topology, lp_id_t from, lp_id_t to, double probability);
extern bool IsNeighbor(lp_id_t from, lp_id_t to, struct topology *topology);


// The following trick belongs to Laurent Deniau at CERN.
// https://groups.google.com/g/comp.std.c/c/d-6Mj5Lko_s?pli=1

/// Compute the number of arguments to a variadic macro
#define PP_NARG(...) \
         PP_NARG_(__VA_ARGS__,PP_RSEQ_N())
/// Compute the number of arguments to a variadic macro
#define PP_NARG_(...) \
         PP_128TH_ARG(__VA_ARGS__)
/// Enumerate the arguments' count to a variadic macro
#define PP_128TH_ARG( \
          _1, _2, _3, _4, _5, _6, _7, _8, _9,_10, \
         _11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
         _21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
         _31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
         _41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
         _51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
         _61,_62,_63,_64,_65,_66,_67,_68,_69,_70, \
         _71,_72,_73,_74,_75,_76,_77,_78,_79,_80, \
         _81,_82,_83,_84,_85,_86,_87,_88,_89,_90, \
         _91,_92,_93,_94,_95,_96,_97,_98,_99,_100, \
         _101,_102,_103,_104,_105,_106,_107,_108,_109,_110, \
         _111,_112,_113,_114,_115,_116,_117,_118,_119,_120, \
         _121,_122,_123,_124,_125,_126,_127,N,...) N
/// Enumerate the arguments' count to a variadic macro in reverse order
#define PP_RSEQ_N() \
         127,126,125,124,123,122,121,120, \
         119,118,117,116,115,114,113,112,111,110, \
         109,108,107,106,105,104,103,102,101,100, \
         99,98,97,96,95,94,93,92,91,90, \
         89,88,87,86,85,84,83,82,81,80, \
         79,78,77,76,75,74,73,72,71,70, \
         69,68,67,66,65,64,63,62,61,60, \
         59,58,57,56,55,54,53,52,51,50, \
         49,48,47,46,45,44,43,42,41,40, \
         39,38,37,36,35,34,33,32,31,30, \
         29,28,27,26,25,24,23,22,21,20, \
         19,18,17,16,15,14,13,12,11,10, \
         9,8,7,6,5,4,3,2,1,0

extern struct topology *vInitializeTopology(enum topology_geometry geometry, int argc, ...);


/**
 * @brief Initialize a topology struct
 *
 * @param geometry The topology struct to initialize
 * @param ... The number of regions and/or the number of links, depending on the topology geometry
 * @return A pointer to the topology structure
 */
#define InitializeTopology(geometry, ...) vInitializeTopology(geometry, PP_NARG(__VA_ARGS__), __VA_ARGS__)
/********* TOPOLOGY LIBRARY ************/

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
