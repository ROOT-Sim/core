/**
 * @file scheduler/process.h
 *
 * @brief LP control blocks
 *
 * This header defines a LP control block, keeping information about both
 * simulation state and execution state as a user thread.
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
 * @author Alessandro Pellegrini
 * @author Roberto Vitali
 *
 * @date November 5, 2013
 */

#pragma once

#include <stdbool.h>

#include <mm/state.h>
<<<<<<< HEAD
#include <mm/slab.h>

#include <mm/mm.h>
#include <exc/allocator/allocator.h>
=======
>>>>>>> origin/incremental
#include <mm/ecs.h>
#include <datatypes/list.h>
#include <datatypes/msgchannel.h>
#include <arch/ult.h>
#include <lib/numerical.h>
#include <lib/abm_layer.h>
#include <lib/topology.h>
#include <communication/communication.h>
<<<<<<< HEAD
#include <arch/x86/linux/rootsim/ioctl.h>
=======
#include <arch/linux/modules/cross_state_manager/cross_state_manager.h>
#include <core/power.h>
>>>>>>> origin/power

#define LP_STACK_SIZE	4194304	// 4 MB

#define LP_STATE_READY			0x00001
#define LP_STATE_RUNNING		0x00002
<<<<<<< HEAD
#define LP_STATE_RUNNING_ECS		0x00004
#define LP_STATE_ROLLBACK		0x00008
#define LP_STATE_SILENT_EXEC		0x00010
#define LP_STATE_ROLLBACK_ALLOWED	0x00020
<<<<<<< HEAD
=======
#define LP_STATE_READY_FOR_SYNCH	0x00040	// This should be a blocked state! Check schedule() and stf()
>>>>>>> origin/power
#define LP_STATE_SUSPENDED		0x01010
#define LP_STATE_WAIT_FOR_SYNCH		0x01001
#define LP_STATE_WAIT_FOR_UNBLOCK	0x01002
#define LP_STATE_WAIT_FOR_DATA		0x01004
<<<<<<< HEAD
<<<<<<< HEAD
#define LP_STATE_READY_FOR_DATA		0x01005
#define LP_STATE_WAIT_FOR_WRITE		0X01006
=======
#define LP_STATE_WAIT_FOR_ROLLBACK_ACK	0x01008
>>>>>>> origin/asym
=======
#define LP_STATE_ROLLBACK		0x00004
#define LP_STATE_CANCELBACK		0x00100
#define LP_STATE_SILENT_EXEC		0x00008
#define LP_STATE_READY_FOR_SYNCH	0x00010
#define LP_STATE_WAIT_FOR_SYNCH		0x01001
#define LP_STATE_WAIT_FOR_UNBLOCK	0x01002
#define LP_STATE_SYNCH_FOR_CANCELBACK	0x00200

>>>>>>> origin/cancelback
=======
#define LP_STATE_WAIT_FOR_ROLLBACK_ACK	0x01008
>>>>>>> origin/power

#define BLOCKED_STATE			0x01000
#define is_blocked_state(state)	(bool)(state & BLOCKED_STATE)

<<<<<<< HEAD
typedef enum { IDLE, REQUESTED, PROCESSING } rb_status;
=======
struct _revwin_t;
>>>>>>> origin/reverse


struct lp_struct {
	/// LP execution state.
	LP_context_t context;

	/// LP execution state when blocked during the execution of an event
	LP_context_t default_context;

	/// Process' stack
	void *stack;

	/// Memory map of the LP
	struct memory_map mm;
	/// sLaB
	struct slab_chain *slab;

	/// Local ID of the LP
	LID_t lid;

	/// Global ID of the LP
	GID_t gid;

	/// Logical Process lock, used to serialize accesses to concurrent data structures
	spinlock_t	lock;

	/// Seed to generate pseudo-random values
	seed_type	seed;

<<<<<<< HEAD
	/// ID of the worker thread towards which the LP is bound
	unsigned int worker_thread;

    /// ID of the Processing Thread (in case worker_thread above is a controller) which processes events
    unsigned int processing_thread;
=======
	/// ID of the worker thread (or controller) towards which the LP is bound
	unsigned int		worker_thread;
>>>>>>> origin/power

	/// ID of the Processing Thread (in case worker_thread above is a controller) which processes events
	unsigned int		processing_thread;

	/// Current execution state of the LP
	short unsigned int state;

<<<<<<< HEAD
	/// This variable maintains the current checkpointing interval for the LP
	unsigned int ckpt_period;
=======
	/// State to resume after Cancelback execution
	short unsigned int state_to_resume;

	/// This variable mainains the current checkpointing interval for the LP
	unsigned int	ckpt_period;
>>>>>>> origin/cancelback

	/// Counts how many events executed from the last checkpoint (to support PSS)
	unsigned int from_last_ckpt;

	/// Counts how many incremental checkpoints have been taken (to support ISS)
	unsigned int from_last_full_ckpt;

#ifdef HAVE_REVERSE
	/// Counter of how many events should be executed withouth creating a reverse window
	unsigned int	events_in_coasting_forward;

	// This is the list of the reverse events which implement software reversibility
	list(revwin_t)	reverse_events;
#endif

	/// If this variable is set, the next invocation to LogState() takes a new state log, independently of the checkpointing interval
<<<<<<< HEAD
	bool state_log_forced;
=======
	bool		state_log_forced;
>>>>>>> origin/cancelback

	/// If this variable is set, the next invocation to LogState() takes a full checkpointing, independently of the checkpointing mode or policies
	bool state_log_full_forced;

	/// The current state base pointer (updated by SetState())
	void *current_base_pointer;

	/// Input messages queue
	 list(msg_t) queue_in;

	/// Pointer to the last correctly processed event
	msg_t *last_processed;

	///Pointer to the last correctly processed event (to be confirmed by the CT)
	msg_t *next_last_processed;

	/// Pointer to the last scheduled event
	msg_t *bound;

	/// Bound lock
	spinlock_t bound_lock;

	/// Send time of the last event extracted from the output port of the PT executing events of this LP
	simtime_t		last_sent_time;

	/// In case of a rollback operation, this points to the old bound
	msg_t		*old_bound;

	/// Output messages queue
	list(msg_hdr_t) queue_out;

	/// Event retirement queue
	list(msg_t)		retirement_queue;

	/// Saved states queue
	list(state_t) queue_states;

    /// Event retirement queue
    list(msg_t) retirement_queue;

	/// Bottom halves
	msg_channel *bottom_halves;

	/// Processed rendezvous queue
	list(msg_t) rendezvous_queue;

    /// Unique identifier within the LP
    unsigned long long mark;

    /// Variable used to keep track of NOTICES sent to PTs in asymmetrics to
    /// avoid nested rollback processing
    unsigned long long rollback_mark;

    /// A per-LP timer to measure to turnaroud time of a rollback notice/ack message exchange
    clock_t start, end;

	/// Buffer used by KLTs for buffering outgoing messages during the execution of an event
	outgoing_t outgoing_buffer;

<<<<<<< HEAD
<<<<<<< HEAD
=======
    /// Current status of a possible rollback request
	rb_status rollback_status;

>>>>>>> origin/asym
	/**
	 * Implementation of OnGVT used for this LP. This can be changed
	 * at runtime by the autonomic subsystem, when dealing with ISS and SSS
	 */
	bool (*OnGVT)(unsigned int me, void *snapshot);
=======
	#ifdef HAVE_CROSS_STATE
	GID_t			ECS_synch_table[MAX_CROSS_STATE_DEPENDENCIES];
<<<<<<< HEAD
	unsigned int 	ECS_index;
	list(ecs_page_node_t)	ECS_page_list;
	list(ecs_page_node_t)	ECS_prefetch_list;
	simtime_t		ECS_last_prefetch_switch;
	int				ECS_current_prefetch_mode;
	int				ECS_page_faults;
	int				ECS_clustered_faults;
	int				ECS_scattered_faults;
	int				ECS_no_prefetch_time;
	int				ECS_clustered_time;
	int				ECS_scattered_time;
	int				ECS_no_prefetch_events;
	int				ECS_clustered_events;
	int				ECS_scattered_events;

=======
	unsigned int 		ECS_index;
>>>>>>> origin/power
	#endif
>>>>>>> origin/ecs

	/**
	 * Implementation of ProcessEvent used for this LP. This can be changed
	 * at runtime by the autonomic subsystem, when dealing with ISS and SSS
	 */
	void (*ProcessEvent)(unsigned int me, simtime_t now, int event_type,
			     void *event_content, unsigned int size,
			     void *state);

#ifdef HAVE_CROSS_STATE
	GID_t ECS_synch_table[MAX_CROSS_STATE_DEPENDENCIES];
	unsigned int ECS_index;
#endif

<<<<<<< HEAD
<<<<<<< HEAD
	unsigned long long wait_on_rendezvous;
	unsigned int wait_on_object;
=======
	stat_interval_t		interval_stats;
} LP_State;
>>>>>>> origin/power
=======
	unsigned long long	wait_on_rendezvous;
	unsigned int		wait_on_object;

	unsigned int		FCF;
	struct _revwin_t	*current_revwin;
	simtime_t		last_correct_log_time;

} LP_state;
>>>>>>> origin/reverse

	/* Per-Library variables */
	numerical_state_t numerical;

	/// pointer to the topology struct
	topology_t *topology;

	/// pointer to the region struct
	region_abm_t *region;

};

// LPs process control blocks and binding control blocks
extern struct lp_struct **lps_blocks;
extern __thread struct lp_struct **lps_bound_blocks;

// Mask of LPs bounded to threads that are yet to be filled
// in the current execution of asym_schedule. It resets
// to lps_bound_block each time asym_schedule is called.
extern __thread struct lp_struct **asym_lps_mask;

// Mask of LP bound to thread that are yet to be filled 
// in the current execution of asym_schedule. It reset 
// to lps_bound_block each time asym_schedule is called. 
extern __thread LP_State **asym_lps_mask;

/** This macro retrieves the LVT for the current LP. There is a small interval window
 *  where the value returned is the one of the next event to be processed. In particular,
 *  this happens in the scheduling function, when the bound is advanced to the next event to
 *  be processed, just before its actual execution.
 */
//#define lvt(lp) (lp->bound != NULL ? lp->bound->timestamp : 0.0)

<<<<<<< HEAD
// TODO: see issue #121 to see how to make this ugly hack disappear
extern __thread unsigned int __lp_counter;
extern __thread unsigned int __lp_bound_counter;

#define foreach_lp(lp)		__lp_counter = 0;\
				for(struct lp_struct *(lp) = lps_blocks[__lp_counter]; __lp_counter < n_prc && ((lp) = lps_blocks[__lp_counter]); ++__lp_counter)

#define foreach_bound_lp(lp)	__lp_bound_counter = 0;\
				for(struct lp_struct *(lp) = lps_bound_blocks[__lp_bound_counter]; __lp_bound_counter < n_lp_per_thread && ((lp) = lps_bound_blocks[__lp_bound_counter]); ++__lp_bound_counter)

#define foreach_bound_mask_lp(lp)	__lp_bound_counter = 0;\
				for(struct lp_struct *(lp) = asym_lps_mask[__lp_bound_counter]; __lp_bound_counter < n_lp_per_thread && ((lp) = asym_lps_mask[__lp_bound_counter]); ++__lp_bound_counter)
=======
#define LPS(lid) ((__builtin_choose_expr(is_lid(lid), lps_blocks[lid.id], (void)0)))
#define LPS_bound(lid) (__builtin_choose_expr(__builtin_types_compatible_p(__typeof__ (lid), unsigned int), lps_bound_blocks[lid], (void)0))
#define LPS_bound_mask(lid) (__builtin_choose_expr(__builtin_types_compatible_p(__typeof__ (lid), unsigned int), asym_lps_mask[lid], (void)0))

extern inline void LPS_bound_set(unsigned int entry, LP_State *lp_block);
extern inline int LPS_foreach(int (*f)(LID_t, GID_t, unsigned int, void *), void *data);
extern inline int LPS_bound_foreach(int (*f)(LID_t, GID_t, unsigned int, void *), void *data);
extern inline int LPS_asym_mask_foreach(int (*f)(LID_t, GID_t, unsigned int, void *), void *data);
extern void initialize_control_blocks(void);
extern void initialize_binding_blocks(void);
>>>>>>> origin/power

#define LPS_bound_set(entry, lp)	lps_bound_blocks[(entry)] = (lp);

extern void initialize_binding_blocks(void);
extern void free_binding_blocks(void);
extern void initialize_lps(void);
extern void update_last_processed(void);
extern struct lp_struct *find_lp_by_gid(GID_t);