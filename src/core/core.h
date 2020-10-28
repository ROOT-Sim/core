/**
* @file core/core.h
*
* @brief Core ROOT-Sim functionalities
*
* Core ROOT-Sim functionalities
*
* @copyright
* Copyright (C) 2008-2019 HPDCS Group
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
*
* @author Francesco Quaglia
* @author Alessandro Pellegrini
* @author Roberto Vitali
*
* @date 3/18/2011
*/

#pragma once

#include <ROOT-Sim.h>
#include <stdio.h>
#include <limits.h>
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <setjmp.h>


<<<<<<< HEAD
#include <lib/numerical.h>
<<<<<<< HEAD
#include <arch/atomic.h>
<<<<<<< HEAD
=======
=======
#include <arch/thread.h>
#include <statistics/statistics.h>
#include <mm/reverse.h>
>>>>>>> origin/reverse


extern int controller_committed_events;
extern atomic_t final_processed_events;
extern __thread int my_processed_events;
>>>>>>> origin/power


extern int controller_committed_events;
extern atomic_t final_processed_events;
extern __thread int my_processed_events;
=======
#define __visible __attribute__((visibility("default")))

>>>>>>> origin/incremental

/// This macro expands to true if the local kernel is the master kernel
#define master_kernel() (kid == 0)

// XXX: This should be moved to state or queues
enum {
	SNAPSHOT_INVALID = 0,	/**< By convention 0 is the invalid field */
	SNAPSHOT_FULL,		/**< Full State Saving */
	SNAPSHOT_SOFTINC,	/**< Incremental State Saving (software based) */
	SNAPSHOT_HARDINC,	/**< Incremental State Saving (hardware based) */
};

/// Maximum number of kernels the distributed simulator can handle
#define N_KER_MAX	128

/// Maximum number of LPs the simulator will handle
#define MAX_LPs		250000

// XXX: this should be moved somewhere else...
enum {
	VERBOSE_INVALID = 0,	/**< By convention 0 is the invalid field */
	VERBOSE_INFO,		/**< xxx documentation */
	VERBOSE_DEBUG,		/**< xxx documentation */
	VERBOSE_NO		/**< xxx documentation */
};

extern __thread jmp_buf exit_jmp;

/// Optimize the branch as likely taken
#define likely(exp) __builtin_expect(exp, 1)
/// Optimize the branch as likely not taken
#define unlikely(exp) __builtin_expect(exp, 0)


enum {
	LP_DISTRIBUTION_INVALID = 0,	/**< By convention 0 is the invalid field */
	LP_DISTRIBUTION_BLOCK,		/**< Distribute exceeding LPs according to a block policy */
	LP_DISTRIBUTION_CIRCULAR	/**< Distribute exceeding LPs according to a circular policy */
};

// XXX should be moved to a more librarish header
/// Equality condition for floats
#define F_EQUAL(a,b) (fabsf((a) - (b)) < FLT_EPSILON)
/// Equality to zero condition for floats
#define F_EQUAL_ZERO(a) (fabsf(a) < FLT_EPSILON)
/// Difference condition for floats
#define F_DIFFER(a,b) (fabsf((a) - (b)) >= FLT_EPSILON)
/// Difference from zero condition for floats
#define F_DIFFER_ZERO(a) (fabsf(a) >= FLT_EPSILON)

/// Equality condition for doubles
#define D_EQUAL(a,b) (fabs((a) - (b)) < DBL_EPSILON)
/// Equality to zero condition for doubles
#define D_EQUAL_ZERO(a) (fabs(a) < DBL_EPSILON)
/// Difference condition for doubles
#define D_DIFFER(a,b) (fabs((a) - (b)) >= DBL_EPSILON)
/// Difference from zero condition for doubles
#define D_DIFFER_ZERO(a) (fabs(a) >= DBL_EPSILON)

/// Macro to find the maximum among two values
#ifdef max
#undef max
#endif
#define max(a, b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

/// Macro to find the minimum among two values
#ifdef min
#undef min
#endif
#define min(a, b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

/// Macro to "legitimately" pun a type
#define UNION_CAST(x, destType) (((union {__typeof__(x) a; destType b;})x).b)

// GID and LID types

/**
 * @brief Definition of a GID.
 *
 * This structure defines a GID. The purpose of this structure is to make
 * functions dealing with GIDs and LIDs type safe, to avoid runtime problems
 * if the two are mixed when calling a function.
 */
typedef struct _gid_t {
	unsigned int to_int;	///< The GID numerical value
} GID_t;


/**
 * @brief Definition of a LID.
 *
 * This structure defines a LID. The purpose of this structure is to make
 * functions dealing with GIDs and LIDs type safe, to avoid runtime problems
 * if the two are mixed when calling a function.
 */
typedef struct _lid_t {
	unsigned int to_int;	///< The LID numerical value
} LID_t;

#define is_lid(val) __builtin_types_compatible_p(__typeof__ (val), LID_t)
#define is_gid(val) __builtin_types_compatible_p(__typeof__ (val), GID_t)

<<<<<<< HEAD
#define lid_equals(first, second) (is_lid(first) && is_lid(second) && first.to_int == second.to_int)
#define gid_equals(first, second) (is_gid(first) && is_gid(second) && first.to_int == second.to_int)
=======
typedef enum {positive, negative, positive_cb, negative_cb, other} message_kind_t;
>>>>>>> origin/cancelback

//#define lid_to_int(lid) __builtin_choose_expr(is_lid(lid), (lid).to_int, (void)0)
//#define gid_to_int(gid) __builtin_choose_expr(is_gid(gid), (gid).to_int, (void)0)

#define set_lid(lid, value) (__builtin_choose_expr(is_lid(lid), lid.to_int, (void)0) = (value))
#define set_gid(gid, value) (__builtin_choose_expr(is_gid(gid), gid.to_int, (void)0) = (value))

typedef enum { positive, negative, control } message_kind_t;

#ifdef HAVE_MPI
typedef unsigned char phase_colour;
#endif

#define MSG_PADDING offsetof(msg_t, sender)
#define MSG_META_SIZE (offsetof(msg_t, event_content) - MSG_PADDING)

struct _state_t;

/// Message Type definition
typedef struct _msg_t {

	/* Place here all members of the struct which should not be transmitted over the network */
<<<<<<< HEAD

#ifdef HAVE_APPROXIMATED_ROLLBACK
	bool is_approximated;
#endif
=======
>>>>>>> origin/asym

<<<<<<< HEAD
	// Pointers to attach messages to chains
	struct _msg_t *next;
	struct _msg_t *prev;
    bool	unprocessed;
=======
	struct _msg_t 		*next;
	struct _msg_t 		*prev;
	unsigned int		alloc_tid; // TODO: this should be moved into an external container, to avoid transmitting it!
	bool			unprocessed;
>>>>>>> origin/power

	/* Place here all members which must be transmitted over the network. It is convenient not to reorder the members
	 * of the structure. If new members have to be added, place them right before the "Model data" part.*/

	// Kernel's information
<<<<<<< HEAD
	GID_t sender;
	GID_t receiver;
#ifdef HAVE_MPI
	phase_colour colour;
#endif
	int type;
	message_kind_t message_kind;
	simtime_t timestamp;
	simtime_t send_time;
	unsigned long long mark;	/// Unique identifier of the message, used for antimessages
	unsigned long long rendezvous_mark;	/// Unique identifier of the message, used for rendez-vous events

	unsigned int sample_id;
	// Model data
	size_t size;
	unsigned char event_content[];
=======
	unsigned int   		sender;
	unsigned int   		receiver;
	int   			type;
	simtime_t		timestamp;
	simtime_t		send_time;
	// TODO: risistemare questa cosa degli antimessaggi
	message_kind_t		message_kind;
	bool			marked_by_antimessage;
	unsigned long long	mark;	/// Unique identifier of the message, used for antimessages
	unsigned long long	rendezvous_mark;	/// Unique identifier of the message, used for rendez-vous events
<<<<<<< HEAD
    //	struct _state_t 	*is_first_event_of;
	// Application informations
	char event_content[MAX_EVENT_SIZE];
	int size;
>>>>>>> origin/cancelback
} msg_t;

=======
	struct _state_t		*checkpoint_of_event;  /// This is used to keep a pointer to the checkpoint taken after the execution of an event. It's NULL if no checkpoint was taken
	// Application informations
	char event_content[MAX_EVENT_SIZE];
	int size;

#ifdef HAVE_REVERSE
	// Reverse window
	revwin_t *revwin;
#endif

} msg_t;



>>>>>>> origin/reverse
/// Message envelope definition. This is used to handle the output queue and stores information needed to generate antimessages
typedef struct _msg_hdr_t {
	// Pointers to attach messages to chains
	struct _msg_hdr_t *next;
	struct _msg_hdr_t *prev;
	// Kernel's information
	GID_t sender;
	GID_t receiver;
	// TODO: non serve davvero, togliere
	int type;
	unsigned long long rendezvous_mark;	/// Unique identifier of the message, used for rendez-vous event
	// TODO: fine togliere
	simtime_t timestamp;
	simtime_t send_time;
	unsigned long long mark;
} msg_hdr_t;


// XXX: this should be refactored someway
<<<<<<< HEAD
extern unsigned int kid,	/* Kernel ID for the local kernel */
 n_ker,				/* Total number of kernel instances */
 n_cores,			/* Total number of cores required for simulation */
 n_prc,				/* Number of LPs hosted by the current kernel instance */
*kernel;

<<<<<<< HEAD
/// Current number of active threads. This is always <= n_cores
extern volatile unsigned int active_threads;
=======
/// Configuration of the execution of the simulator
typedef struct _simulation_configuration {
	char *output_dir;		/// Destination Directory of output files
	int backtrace;			/// Debug mode flag
	int scheduler;			/// Which scheduler to be used
	int gvt_time_period;		/// Wall-Clock time to wait before executiong GVT operations
	int gvt_snapshot_cycles;	/// GVT operations to be executed before rebuilding the state
	int simulation_time;		/// Wall-clock-time based termination predicate
	int lps_distribution;		/// Policy for the LP to Kernel mapping
	int ckpt_mode;			/// Type of checkpointing mode (Synchronous, Semi-Asyncronous, ...)
	int checkpointing;		/// Type of checkpointing scheme (e.g., PSS, CSS, ...)
	int ckpt_period;		/// Number of events to execute before taking a snapshot in PSS (ignored otherwise)
	int snapshot;			/// Type of snapshot (e.g., full, incremental, autonomic, ...)
	int check_termination_mode;	/// Check termination strategy: standard or incremental
	bool blocking_gvt;		/// GVT protocol blocking or not
	bool deterministic_seed;	/// Does not change the seed value config file that will be read during the next runs
	int verbose;			/// Kernel verbose
	enum stat_levels stats;		/// Produce performance statistic file (default STATS_ALL)
	bool serial;			// If the simulation must be run serially
	seed_type set_seed;		/// The master seed to be used in this run

#ifdef HAVE_PREEMPTION
	bool disable_preemption;	/// If compiled for preemptive Time Warp, it can be disabled at runtime
#endif

#ifdef HAVE_PARALLEL_ALLOCATOR
	bool disable_allocator;
#endif

#ifdef HAVE_REVERSE
	bool disable_reverse;
#endif
} simulation_configuration;


/// Barrier for all worker threads
extern barrier_t all_thread_barrier;
>>>>>>> origin/reverse

extern void ProcessEvent_light(unsigned int me, simtime_t now, int event_type, void *event_content, unsigned int size, void *state);
bool OnGVT_light(unsigned int me, void *snapshot);
extern void ProcessEvent_inc(unsigned int me, simtime_t now, int event_type, void *event_content, unsigned int size, void *state);
bool OnGVT_inc(unsigned int me, void *snapshot);
extern void RestoreApproximated(void *lp_state);
=======
extern unsigned int	kid,		/* Kernel ID for the local kernel */
			n_ker,		/* Total number of kernel instances */
			n_cores,	/* Total number of cores required for simulation */
			n_prc,		/* Number of LPs hosted by the current kernel instance */
			* kernel;

<<<<<<< HEAD
extern void ProcessEvent(unsigned int me, simtime_t now, int event_type, void *event_content, unsigned int size, void *state);
bool OnGVT(unsigned int me, void *snapshot);
extern void ProcessEvent_instr(unsigned int me, simtime_t now, int event_type, void *event_content, unsigned int size, void *state);
bool OnGVT_instr(unsigned int me, void *snapshot);
>>>>>>> origin/incremental
=======

extern bool mpi_is_initialized;

extern simulation_configuration rootsim_config;

extern void ProcessEvent_reverse(unsigned int me, simtime_t now, int event_type, void *event_content, unsigned int size, void *state);
bool OnGVT_reverse(int gid, void *snapshot);
extern void ProcessEvent(unsigned int me, simtime_t now, int event_type, void *event_content, unsigned int size, void *state);
bool OnGVT(int gid, void *snapshot);
extern void ProcessEvent_inc(unsigned int me, simtime_t now, int event_type, void *event_content, unsigned int size, void *state);
bool OnGVT_inc(int gid, void *snapshot);
extern bool (**_OnGVT)(int gid, void *snapshot);
extern void (**_ProcessEvent)(unsigned int me, simtime_t now, int event_type, void *event_content, unsigned int size, void *state);
>>>>>>> origin/reverse

extern void base_init(void);
extern void base_fini(void);
extern unsigned int find_kernel_by_gid(GID_t gid) __attribute__((pure));
extern void _rootsim_error(bool fatal, const char *msg, ...);
extern void distribute_lps_on_kernels(void);
extern void simulation_shutdown(int code) __attribute__((noreturn));
extern inline bool user_requested_exit(void);
extern inline bool simulation_error(void);
extern void initialization_complete(void);
extern bool end_computing(void);


#define rootsim_error(fatal, msg, ...) _rootsim_error(fatal, "%s:%d: %s(): " msg, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
