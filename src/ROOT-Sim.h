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
 * SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <float.h>
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

/**
 * @brief Perform output operations.
 * @param me The logical process ID of the LP performing the output
 * @param output_type Numerical output type
 * @param output_content The output content
 * @param output_size The size (in bytes) of the output content
 *
 * This function is called by the simulation kernel whenever an event is committed, during the handling of which
 * an output operation had been scheduled by the simulation model using the ScheduleOutput() function.
 * This function will receive the same data that was passed to the ScheduleOutput() function.
 *
 * @warning The memory pointed by output_content is not a copy, and is owned by the simulation kernel. It will be
 * freed by the simulation kernel after the function returns.
 * @warning The function shall not perform any memory-managed allocation (e.g. using rs_malloc).
 */
typedef void (*OutputCallback_t)(lp_id_t me, unsigned output_type, const void *output_content, unsigned output_size);

/// @brief Internal event types used by the simulation kernel.
///
/// These event types are automatically scheduled to the model according to the following logic:
/// - `LP_INIT`: Represents the initialization event for a logical process.
/// - `LP_FINI`: Represents the finalization event for a logical process.
enum rootsim_event { LP_INIT = 65534, LP_FINI };

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

/**
 * @brief API to schedule a new output event to execute once the event being currently executed is committed
 *
 * This function makes a copy of the data in @p output_content , which stays owned by the caller.
 *
 * @param output_type Numerical output type to be passed to the output handling function
 * @param output_content The output content
 * @param output_size The size (in bytes) of the output content
 *
 * @warning This function shall not be called during the handling of LP_FINI events, as it will produce no output.
 */
extern void ScheduleOutput(unsigned output_type, const void *output_content, unsigned output_size);

/**
 * @brief Allocates rollbackable memory
 *
 * This function is part of the custom memory management system and is used to allocate
 * memory dynamically. The allocated memory is not initialized. Upon a rollback, the previous
 * content of the allocated memory buffer is restored to the previous (consistent) content.
 *
 * @param req_size The size of the memory block to allocate, in bytes.
 * @return A pointer to the allocated memory block, or `NULL` if the allocation fails.
 *
 * @note If `req_size` is 0, the function returns `NULL`.
 * @warning The returned memory _must_ be freed using `rs_free()`.
 */
extern void *rs_malloc(size_t req_size);

/**
 * @brief Allocates and zero-initializes rollbackable memory
 *
 * This function is part of the custom memory management system and is used to allocate
 * memory dynamically. The allocated memory is initialized to zero. Upon a rollback, the
 * previous content of the allocated memory buffer is restored to the previous (consistent) content.
 *
 * @param nmemb The number of elements to allocate.
 * @param size The size of each element, in bytes.
 * @return A pointer to the allocated memory block, or `NULL` if the allocation fails.
 *
 * @note If `nmemb` or `size` is 0, the function returns `NULL`.
 * @warning The returned memory _must_ be freed using `rs_free()`.
 */
extern void *rs_calloc(size_t nmemb, size_t size);

/**
 * @brief Frees rollbackable memory
 *
 * This function is part of the custom memory management system and is used to free
 * memory that was previously allocated using `rs_malloc()`, `rs_calloc()`, or `rs_realloc()`.
 * Upon a rollback, the memory is restored to its previous (consistent) state. This means that
 * also the address of the free'd buffer will be the same. Linked data structures can be therefore
 * safely implemented in the model, as internal pointers will be valid after a rollback.
 *
 * @param ptr A pointer to the memory block to be freed. If `ptr` is `NULL`, no operation is performed.
 *
 * @warning The memory block must have been allocated using the custom memory management functions.
 */
extern void rs_free(void *ptr);

/**
 * @brief Reallocates rollbackable memory
 *
 * This function is part of the custom memory management system and is used to resize
 * a previously allocated memory block. If the reallocation is successful, the content
 * of the memory block is preserved up to the minimum of the old and new sizes.
 * Upon a rollback, the memory is restored to its previous (consistent) state.
 *
 * @param ptr A pointer to the memory block to be reallocated. If `ptr` is `NULL`, the function behaves like
 *            `rs_malloc()`.
 * @param req_size The new size of the memory block, in bytes.
 * @return A pointer to the reallocated memory block, or `NULL` if the reallocation fails.
 *
 * @note If `req_size` is 0, the function frees the memory block and returns `NULL`.
 * @warning The memory block must have been allocated using the custom memory management functions.
 */
extern void *rs_realloc(void *ptr, size_t req_size);

/**
 * @brief Logging levels used by the simulation kernel.
 *
 * These levels define the verbosity of log messages emitted during the simulation.
 */
enum log_level {
	LOG_TRACE, //!< The logging level reserved for very low priority messages.
	LOG_DEBUG, //!< The logging level reserved for useful debug messages.
	LOG_INFO,  //!< The logging level reserved for useful runtime messages.
	LOG_WARN,  //!< The logging level reserved for unexpected, non-deal-breaking conditions.
	LOG_ERROR, //!< The logging level reserved for unexpected, problematic conditions.
	LOG_FATAL, //!< The logging level reserved for unexpected, fatal conditions.
	LOG_SILENT //!< Emit no messages during the simulation.
};

enum synchronization_algorithm {
	SERIAL = 1, //!< The simulation runs on the serial runtime
	TIME_WARP, //!< The simulation runs using the optimistic Time Warp algorithm
};

/// A set of configurable values used by other modules
struct simulation_configuration {
	/// The number of LPs to be used in the simulation
	lp_id_t lps;
	/// The number of threads to be used in the simulation. If zero, it defaults to the amount of available cores
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
	/// If set, worker threads are bound to physical cores
	bool core_binding;
	/// Specify what synchronization algorithm we are using
	enum synchronization_algorithm synchronization;
	/**
	 * @deprecated since 3.1.0
	 * If set, the simulation will run on the serial runtime
	 */
	bool serial;
	/// Function pointer to the dispatching function
	ProcessEvent_t dispatcher;
	/// Function pointer to the termination detection function
	CanEnd_t committed;
	/// Function pointer to the output handling function
	OutputCallback_t output_callback;
};

extern int RootsimInit(const struct simulation_configuration *conf);
extern int RootsimRun(void);
extern void RootsimStop(void);
