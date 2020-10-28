/**
 * @file scheduler/scheduler.c
 *
 * @brief The ROOT-Sim scheduler main module
 *
 * This module implements the schedule() function, which is the main
 * entry point for all the schedulers implemented in ROOT-Sim, and
 * several support functions which allow to initialize worker threads.
 *
 * Also, the LP_main_loop() function, which is the function where all
 * the User-Level Threads associated with Logical Processes live, is
 * defined here.
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
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
<<<<<<< HEAD
=======

#include <mm/dymelor.h>
>>>>>>> origin/reverse
#include <datatypes/list.h>
#include <datatypes/msgchannel.h>
#include <communication/communication.h>
#include <core/core.h>
#include <core/timer.h>
#include <arch/atomic.h>
#include <arch/ult.h>
#include <arch/thread.h>
#include <exc/allocator/allocator.h>
#include <core/init.h>
<<<<<<< HEAD
#include <mm/dymelor.h>
=======
#include <gvt/ccgs.h>
>>>>>>> origin/termination
#include <scheduler/binding.h>
#include <scheduler/ht_sched.h>
#include <scheduler/process.h>
#include <scheduler/scheduler.h>
#include <scheduler/stf.h>
#include <mm/state.h>
<<<<<<< HEAD
=======
#include <statistics/statistics.h>
#include <arch/thread.h>
>>>>>>> origin/reverse
#include <communication/communication.h>
#include <datatypes/heap.h>

#ifdef HAVE_CROSS_STATE
#include <mm/ecs.h>
#endif

#include <mm/mm.h>
#include <statistics/statistics.h>
#include <gvt/gvt.h>
<<<<<<< HEAD
#include <arch/x86/linux/cross_state_manager/cross_state_manager.h>
=======
#include <statistics/statistics.h>
<<<<<<< HEAD
#include <arch/x86/linux/rootsim/ioctl.h>
>>>>>>> origin/incremental
=======
#include <mm/reverse.h>

#ifdef EXTRA_CHECKS
>>>>>>> origin/reverse
#include <queues/xxhash.h>
#include <score/score.h>

<<<<<<< HEAD
=======
// static __thread atomic_t notice_count;

/// This is used to keep track of how many LPs were bound to the current KLT
__thread unsigned int n_prc_per_thread;
>>>>>>> origin/power

#ifdef HAVE_PMU
#include <sys/ioctl.h>  /* ioctl syscall*/
#include <unistd.h>	/* close syscall */
#endif
/// This is used to keep track of how many LPs were bound to the current KLT
__thread unsigned int n_lp_per_thread;

/// This is a per-thread variable pointing to the block state of the LP currently scheduled
__thread struct lp_struct *current;

/**
 * This is a per-thread variable telling what is the event that should be executed
 * when activating an LP. It is incorrect to rely on current->bound, as there
 * are cases (such as the silent execution) in which we have a certain bound set,
 * but we execute a different event.
 *
 * @todo We should uniform this behaviour, and drop current_evt, as this might be
 *       misleading when reading the code.
 */
__thread msg_t *current_evt;

<<<<<<< HEAD
// Timer per thread used to gather statistics on execution time for
// controllers and processers in asymmetric executions
//static __thread timer timer_local_thread;

// Pointer to an array of longs which are used as an accumulator of time
// spent idle in asym_schedule or asym_process
long *total_idle_microseconds;
=======
#ifdef HAVE_PMU
__thread int fd = 0;
__thread memory_trace_t memory_trace;
__thread bool pmu_enabled = false;
#endif

<<<<<<< HEAD
>>>>>>> origin/incremental
=======
/// This is a list to keep high-priority messages yet to be processed
static __thread list(msg_t) hi_prio_list;

// Timer per thread used to gather statistics on execution time for 
// controllers and processers in asymmetric executions
static __thread timer timer_local_thread;

// Pointer to an array of longs which are used as an accumulator of time 
// spent idle in asym_schedule or asym_process
long *total_idle_microseconds;

>>>>>>> origin/power
/*
* This function initializes the scheduler. In particular, it relies on MPI to broadcast to every simulation kernel process
* which is the actual scheduling algorithm selected.
*
* @author Francesco Quaglia
*
* @param sched The scheduler selected initially, but master can decide to change it, so slaves must rely on what master send to them
*/
<<<<<<< HEAD
void scheduler_init(void)
{
#ifdef HAVE_PMU
	if (rootsim_config.snapshot == SNAPSHOT_HARDINC && fd < 0) {
		printf("Error, %s is not available\n", "/dev/rootsim");
		abort();
	}
#endif
#ifdef HAVE_PREEMPTION
	preempt_init();
#endif
=======
void scheduler_init(void) {

	register unsigned int i;

	// TODO: implementare con delle broadcast!!
/*	if(n_ker > 1) {
		if (master_kernel()) {
			for (i = 1; i < n_ker; i++) {
				comm_send(&rootsim_config.scheduler, sizeof(rootsim_config.scheduler), MPI_CHAR, i, MSG_INIT_MPI, MPI_COMM_WORLD);
			}
		} else {
			comm_recv(&rootsim_config.scheduler, sizeof(rootsim_config.scheduler), MPI_CHAR, 0, MSG_INIT_MPI, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		}
	}
*/
	// Allocate LPS control blocks
	LPS = (LP_state **)rsalloc(n_prc * sizeof(LP_state *));
	for (i = 0; i < n_prc; i++) {
		LPS[i] = (LP_state *)rsalloc(sizeof(LP_state));
		bzero(LPS[i], sizeof(LP_state));

		// Allocate memory for the outgoing buffer
		LPS[i]->outgoing_buffer.max_size = INIT_OUTGOING_MSG;
		LPS[i]->outgoing_buffer.outgoing_msgs = rsalloc(sizeof(msg_t) * INIT_OUTGOING_MSG);
	}

	// Initialize the INIT barrier
	barrier_init(&INIT_barrier, n_cores);

}



static void destroy_LPs(void) {
	register unsigned int i;

	for(i = 0; i < n_prc; i++) {
		rsfree(LPS[i]->queue_in);
		rsfree(LPS[i]->queue_out);
		rsfree(LPS[i]->queue_states);
		rsfree(LPS[i]->bottom_halves);
<<<<<<< HEAD
=======
		rsfree(LPS[i]->rendezvous_queue);

#ifdef HAVE_REVERSE
	//	rsfree(LPS[i]->reverse_windows);
#endif
>>>>>>> origin/reverse

		// Destroy stacks
		#ifdef ENABLE_ULT
		lp_free(LPS[i]->stack);
		#endif
	}

>>>>>>> origin/cancelback
}

/**
* This function finalizes the scheduler
*
* @author Alessandro Pellegrini
*/
void scheduler_fini(void)
{
#ifdef HAVE_PREEMPTION
	preempt_fini();
#endif

	foreach_lp(lp) {
		rsfree(lp->queue_in);
		rsfree(lp->queue_out);
		rsfree(lp->queue_states);
		rsfree(lp->bottom_halves);
		rsfree(lp->rendezvous_queue);

		// Destroy stacks
		rsfree(lp->stack);

		rsfree(lp);
	}

	rsfree(lps_blocks);
	rsfree(lps_bound_blocks);
}

/**
* This is a LP main loop. It s the embodiment of the userspace thread implementing the logic of the LP.
* Whenever an event is to be scheduled, the corresponding metadata are set by the schedule() function,
* which in turns calls activate_LP() to execute the actual context switch.
* This ProcessEvent wrapper explicitly returns control to simulation kernel user thread when an event
* processing is finished. In case the LP tries to access state data which is not belonging to its
* simulation state, a SIGSEGV signal is raised and the LP might be descheduled if it is not safe
* to perform the remote memory access. This is the only case where control is not returned to simulation
* thread explicitly by this wrapper.
*
* @param args arguments passed to the LP main loop. Currently, this is not used.
*/
<<<<<<< HEAD
void LP_main_loop(void *args) {
#ifdef EXTRA_CHECKS
=======
static void LP_main_loop(void *args) {
	int delta_event_timer;

	#ifdef EXTRA_CHECKS
>>>>>>> origin/reverse
	unsigned long long hash1, hash2;
	hash1 = hash2 = 0;
#endif

<<<<<<< HEAD
	(void)args;		// this is to make the compiler stop complaining about unused args
=======
	msg_t * prev_event;

	(void)args; // this is to make the compiler stop complaining about unused args
>>>>>>> origin/reverse

	// Save a default context
	context_save(&current->default_context);

<<<<<<< HEAD
	while (true) {
=======

	while(true) {
>>>>>>> origin/reverse

#ifdef EXTRA_CHECKS
		if (current->bound->size > 0) {
			hash1 = XXH64(current_evt->event_content, current_evt->size, current->gid.to_int);
		}
#endif
		allocator_processing_start(current);

		idle_thread_activate();

<<<<<<< HEAD
		timer event_timer;
		timer_start(event_timer);
#ifdef HAVE_APPROXIMATED_ROLLBACK
		event_approximation_mark(current, current_evt);
#endif
		// Process the event
<<<<<<< HEAD
<<<<<<< HEAD
        if(&abm_settings){
=======
on_process_event_forward(current_evt);
		if(&abm_settings){
>>>>>>> origin/energy_tmp
=======
		if(&abm_settings) {
>>>>>>> origin/incremental
			ProcessEventABM();
		} else if (&topology_settings) {
			ProcessEventTopology();
		} else {
			switch_to_application_mode();
			current->ProcessEvent(current->gid.to_int,
				      current_evt->timestamp,
				      current_evt->type,
				      current_evt->event_content,
				      current_evt->size,
				      current->current_base_pointer);
			switch_to_platform_mode();
		}
<<<<<<< HEAD
		
		int delta_event_timer = timer_value_micro(event_timer);
=======
=======
		#ifdef HAVE_REVERSE
		if(!rootsim_config.disable_reverse) {

			// TODO: change this check to account for the model
			if(1 || LPS[current_lp]->from_last_ckpt >= LPS[current_lp]->events_in_coasting_forward) {
			
		
			//if(LPS[current_lp]->from_last_ckpt >= 3 * (LPS[current_lp]->ckpt_period / 4)) {
			//if(LPS[current_lp]->from_last_ckpt >=  (LPS[current_lp]->ckpt_period / 4)) {
			//if(LPS[current_lp]->from_last_ckpt >= (0.66 * LPS[current_lp]->ckpt_period )) {
			//if(LPS[current_lp]->from_last_ckpt >= (0.25 * LPS[current_lp]->ckpt_period )) {
			//	printf("from last is %d - period is %d - barrier is %d\n",(LPS[current_lp]->from_last_ckpt),LPS[current_lp]->ckpt_period, 3*( LPS[current_lp]->ckpt_period/4) );
			//	_ProcessEvent[current_lp] = ProcessEvent_reverse;//this is for FULL REVERSE or REVERSE
				_ProcessEvent[current_lp] = ProcessEvent;//this is for the FCF_path

				// Create a new revwin to bind to the current event and bind it. A revwin could be possibly
				// already allocated, in case an event was undone by a rollback operation. In that case we
				// reset the revwin, rather than free'ing it, so a buffer could be already allocated.
				if(current_evt->revwin == NULL)
					current_evt->revwin = revwin_create();
				else
					revwin_reset(current_lp, current_evt->revwin);

				prev_event = list_prev(current_evt);

				if(prev_event){
					current_evt->revwin->prev = prev_event->revwin; 
				}
				else{
					current_evt->revwin->prev = NULL;
				}

				LPS[current_lp]->current_revwin = current_evt->revwin;

			} else {
				_ProcessEvent[current_lp] = ProcessEvent;

				// this stuff below s for mixed model
				current_evt->revwin = NULL; 
				LPS[current_lp]->current_revwin = NULL;
			}
		}
		#endif

		// Process the event
		timer event_timer;
		timer_start(event_timer);

		if(current_evt->marked_by_antimessage) {
			printf("ERROR in SCHEDULER: %p (%d, %f) --> marked\n", current_evt, current_evt->type, current_evt->timestamp);
			fflush(stdout);
		}

		switch_to_application_mode();

		_ProcessEvent[current_lp](LidToGid(current_lp), current_evt->timestamp, current_evt->type, current_evt->event_content, current_evt->size, current_state);

		switch_to_platform_mode();
>>>>>>> origin/reverse

		delta_event_timer = timer_value_micro(event_timer);

#ifdef HAVE_REVERSE
		revwin_flush_cache();
#endif

		idle_thread_deactivate();

#ifdef HAVE_PMU
		if (rootsim_config.snapshot == SNAPSHOT_HARDINC) {
			size_t size, i;
			do {
				size = ioctl(fd, IOCTL_GET_MEM_TRACE, &memory_trace);
				for (i = 0; i < size; ++i)
					mark_mem((void *)memory_trace.addresses[i], 1);
			} while(size > 0 && size == memory_trace.length);
		}
#endif

>>>>>>> origin/incremental
#ifdef EXTRA_CHECKS
		if (current->bound->size > 0) {
			hash2 =
			    XXH64(current_evt->event_content, current_evt->size, current->gid.to_int);
		}

		if (hash1 != hash2) {
			rootsim_error(true,
				      "Error, LP %src/scheduler/.deps/scheduler.Tpod has modified the payload of event %d during its processing. Aborting...\n",
				      current->gid, current->bound->type);
		}
#endif

<<<<<<< HEAD
		statistics_post_data(current, STAT_EVENT, 1.0);
		statistics_post_data(current, STAT_EVENT_TIME,
				     delta_event_timer);
		//on_process_event_forward(current_evt);
		// Give back control to the simulation kernel's user-level thread
		context_switch(&current->context, &kernel_context);
=======
		#ifdef ENABLE_ULT
		// Give back control to the simulation kernel's user-level thread
		context_switch(&LPS[current_lp]->context, &kernel_context);
		#else
		return;
		#endif
>>>>>>> origin/reverse
	}
}

<<<<<<< HEAD
<<<<<<< HEAD
void initialize_worker_thread(void)
{
	msg_t *init_event;
=======





/**
 * This function initializes a LP execution context. It allocates page-aligned memory for efficiency
 * reasons, and then calls <context_create>() which does the final trick.
 * <context_create>() uses global variables: LPs must therefore be intialized before creating new kernel threads
 * for supporting concurrent execution of LPs.
 *
 * @author Alessandro Pellegrini
 *
 * @date November 8, 2013
 *
 * @param lp the idex of the LP in the LPs descriptor table to be initialized
 */
void initialize_LP(LID_t lp) {
	unsigned int i;

	// Allocate LP stack
	#ifdef ENABLE_ULT
	LPS(lp)->stack = get_ult_stack(LP_STACK_SIZE);
	#endif


	// Set the initial checkpointing period for this LP.
	// If the checkpointing period is fixed, this will not change during the
	// execution. Otherwise, new calls to this function will (locally) update this.
	set_checkpoint_period(lp, rootsim_config.ckpt_period);

	#ifdef HAVE_REVERSE
	// We must execute some events to decide how to execute, so just start with traditional execution
	LPS[lp]->events_in_coasting_forward = rootsim_config.ckpt_period;
	#endif


	// Initially, every LP is ready
<<<<<<< HEAD
	LPS(lp)->state = LP_STATE_READY;
=======
	LPS[lp]->state = LP_STATE_READY;
>>>>>>> origin/cancelback

	// There is no current state layout at the beginning
	LPS(lp)->current_base_pointer = NULL;

	// Initialize the queues
<<<<<<< HEAD
	LPS(lp)->queue_in = new_list(msg_t);
	LPS(lp)->queue_out = new_list(msg_hdr_t);
	LPS(lp)->queue_states = new_list(state_t);
	LPS(lp)->retirement_queue = new_list(msg_t);
	LPS(lp)->rendezvous_queue = new_list(msg_t);
=======
	LPS[lp]->queue_in = new_list(lp, msg_t);
	LPS[lp]->queue_out = new_list(lp, msg_hdr_t);
	LPS[lp]->queue_states = new_list(lp, state_t);
	LPS[lp]->bottom_halves = new_list(lp, msg_t);
	LPS[lp]->rendezvous_queue = new_list(lp, msg_t);
	LPS[lp]->reverse_events = new_list(lp, revwin_t);
	LPS[lp]->FCF = 0;
	LPS[lp]->current_revwin = NULL;
>>>>>>> origin/reverse

	// Initialize the LP lock
	spinlock_init(&LPS(lp)->lock);

	// No event has been processed so far
	LPS(lp)->bound = NULL;

	LPS(lp)->outgoing_buffer.min_in_transit = rsalloc(sizeof(simtime_t) * n_cores);
	for(i = 0; i < n_cores; i++) {
		LPS(lp)->outgoing_buffer.min_in_transit[i] = INFTY;
	}

	#ifdef HAVE_CROSS_STATE
	// No read/write dependencies open so far for the LP. The current lp is always opened
	LPS(lp)->ECS_index = 0;
	LPS(lp)->ECS_synch_table[0] = LidToGid(lp); // LidToGid for distributed ECS
	LPS(lp)->ECS_page_list = new_list(ecs_page_node_t);
	LPS(lp)->ECS_prefetch_list = new_list(ecs_page_node_t);
	LPS(lp)->ECS_page_faults = 0;
	LPS(lp)->ECS_current_prefetch_mode = NO_PREFETCH;
	LPS(lp)->ECS_last_prefetch_switch = current_lvt;
	LPS(lp)->ECS_no_prefetch_events = 1;
	LPS(lp)->ECS_clustered_events = 1;
	LPS(lp)->ECS_scattered_events = 1;
	#endif

	// Create user thread
	#ifdef ENABLE_ULT
	context_create(&LPS(lp)->context, LP_main_loop, NULL, LPS(lp)->stack, LP_STACK_SIZE);
	#endif
}


void initialize_processing_thread(void) {
	communication_init_thread();
	hi_prio_list = new_list(msg_t);
}


void initialize_worker_thread(void) {
	communication_init_thread();
>>>>>>> origin/ecs
=======
void initialize_worker_thread(void) {
    msg_t *init_event;
>>>>>>> origin/asym

	// Divide LPs among worker threads, for the first time here
	initial_binding();
	if (master_thread() && master_kernel()) {
		printf("Initializing LPs... ");
		fflush(stdout);
	}
<<<<<<< HEAD

    if(rootsim_config.num_controllers == 0) {
        thread_barrier(&all_thread_barrier);
    } else {
        thread_barrier(&controller_barrier);
    }

    if (master_thread() && master_kernel())
        printf("done\n");

    // Schedule an INIT event to the newly instantiated LP
    // We need two separate foreach_bound_lp here, because
    // in this way we are sure that there is at least one
    // event to be used as the bound and we do not have to make
    // any check on null throughout the scheduler code.

    foreach_bound_lp(lp) {
        pack_msg(&init_event, lp->gid, lp->gid, INIT, 0.0, 0.0, 0, NULL);
        init_event->mark = generate_mark(lp);
        list_insert_head(lp->queue_in, init_event);
        lp->state_log_forced = true;
    }

    // Worker Threads synchronization barrier: they all should start working together
	if(rootsim_config.num_controllers == 0) {
		thread_barrier(&all_thread_barrier);
	} else {
		thread_barrier(&controller_barrier);
=======
	// Worker Threads synchronization barrier: they all should start working together
	if(rootsim_config.num_controllers == 0) {
		thread_barrier(&all_thread_barrier);
	} else {
		thread_barrier(&controller_barrier);
	}

	if (master_thread() && master_kernel())
		printf("done\n");

	// Schedule an INIT event to the newly instantiated LP
	// We need two separate foreach_bound_lp here, because
	// in this way we are sure that there is at least one
	// event to be used as the bound and we do not have to make
	// any check on null throughout the scheduler code.
	foreach_bound_lp(lp) {
		pack_msg(&init_event, lp->gid, lp->gid, INIT, 0.0, 0.0, 0, NULL);
		init_event->mark = generate_mark(lp);
		list_insert_head(lp->queue_in, init_event);
>>>>>>> origin/incremental
	}

<<<<<<< HEAD
    foreach_bound_lp(lp) {
        schedule_on_init(lp);
    }
=======
	// Here the init function will initialize a reverse memory region
	// which is managed by a slab allocator.
	#ifdef HAVE_REVERSE
	if(!rootsim_config.disable_reverse)
		reverse_init(REVWIN_SIZE);
	#endif

	// TODO: is this double barrier really needed?
	// Worker Threads synchronization barrier: they all should start working together
	thread_barrier(&all_thread_barrier);
>>>>>>> origin/reverse

	if(rootsim_config.num_controllers == 0) {
		thread_barrier(&all_thread_barrier);
	} else {
		thread_barrier(&controller_barrier);
	}
<<<<<<< HEAD

=======
#ifdef HAVE_PMU
	pmu_enabled = rootsim_config.snapshot == SNAPSHOT_HARDINC;
#endif
	// Worker Threads synchronization barrier: they all should start working together
<<<<<<< HEAD
	thread_barrier(&all_thread_barrier);
>>>>>>> origin/incremental
=======
	if(rootsim_config.num_controllers == 0) {
		thread_barrier(&all_thread_barrier);
	} else {
		thread_barrier(&controller_barrier);
	}
>>>>>>> origin/power

#ifdef HAVE_PREEMPTION
	if (!rootsim_config.disable_preemption)
		enable_preemption();
#endif

}

/**
* This function is the application-level __ProcessEvent() callback entry point.
* It allows to specify which lp must be scheduled, specifying its lvt, its event
* to be executed and its simulation state.
* This provides a general entry point to application-level code, to be used
* if the LP is in forward execution, in coasting forward or in initialization.
*
* @author Alessandro Pellegrini
*
* @date November 11, 2013
*
* @param next_LP A pointer to the lp_struct of the LP which has to be activated
* @param next_evt A pointer to the event to be processed by the LP
*/
void activate_LP(struct lp_struct *next_LP, msg_t *next_evt) {

	// Notify the LP main execution loop of the information to be used for actual simulation
	/*if(next_evt->timestamp<get_last_gvt()){
	    dump_msg_content(next_evt);
	    printf("%d\n",get_last_gvt);
	}*/
	current = next_LP;
	current_evt = next_evt;

     //#ifdef HAVE_PREEMPTION
    //if(!rootsim_config.disable_preemption)
    //  enable_preemption();
    //#endif

    #ifdef HAVE_CROSS_STATE
	// Activate memory view for the current LP
	lp_alloc_schedule();
<<<<<<< HEAD
    #endif
    context_switch(&kernel_context, &next_LP->context);
=======
	#endif

//	if(is_blocked_state(LPS(lp)->state)){
//		rootsim_error(true, "Critical condition: LP %d has a wrong state -> %d. Aborting...\n", gid_to_int(LidToGid(lp)), LPS(lp)->state);
//	}

	#ifdef ENABLE_ULT
	context_switch(&kernel_context, &LPS(lp)->context);
	#else
	LP_main_loop(NULL);
	#endif
>>>>>>> origin/power

    //#ifdef HAVE_PREEMPTION
    //if(!rootsim_config.disable_preemption)
    //        disable_preemption();
    //#endif

    #ifdef HAVE_CROSS_STATE
	// Deactivate memory view for the current LP if no conflict has arisen
	if (!is_blocked_state(next_LP->state)) {
//              printf("Deschedule %d\n",lp);
		lp_alloc_deschedule();
<<<<<<< HEAD
	}
    #endif
	if(rootsim_config.num_controllers > 0){
	    next_LP->next_last_processed = next_evt;
	}
	else
	    next_LP->last_processed = next_evt;
=======
	}L
	#endif
>>>>>>> origin/power


    next_evt->unprocessed = false;      ///CONTROLLARE

    current = NULL;
    current_evt = NULL;
}


bool check_rendevouz_request(struct lp_struct *lp) {
	msg_t *temp_mess;

	if (lp->state != LP_STATE_WAIT_FOR_SYNCH)
		return false;

	if (lp->bound != NULL && list_next(lp->bound) != NULL) {
		temp_mess = list_next(lp->bound);
		return temp_mess->type == RENDEZVOUS_START && lp->wait_on_rendezvous > temp_mess->rendezvous_mark;
	}

	return false;
}


void asym_process_one_event(msg_t *msg) {
    struct lp_struct *LP;
    LP = find_lp_by_gid(msg->receiver);

    //spin_lock(&LP->bound_lock); //Process this event
    activate_LP(LP, msg);
    //spin_unlock(&LP->bound_lock);

    asym_send_outgoing_msgs(LP); //Send back to the controller the (possibly) generated events
    LogState(LP);
}


void find_a_match(msg_t *lo_prio_msg) {

    msg_t *hi_prio_msg;
    msg_t *rb_ack;
    int type;

    while(1) {
        hi_prio_msg = pt_get_hi_prio_msg();
        validate_msg(hi_prio_msg);

            type = hi_prio_msg->type;
            if (is_control_msg(type)){
                if(type != ASYM_ROLLBACK_NOTICE){
                    fprintf(stderr, "\tERROR: Type %d CONTROL message SHOULDN'T stay in the HI_PRIO queue!\n",
                            hi_prio_msg->type);
                    dump_msg_content(hi_prio_msg);
                    fflush(stdout);
                    abort();
                }
                else {   //IT IS A NOTICE

                    if(lo_prio_msg->receiver.to_int != hi_prio_msg->receiver.to_int){   //DIFFERENT RECEIVERS
                        printf("\tWARNING: lo/hi prio messages have DIFFERENT receivers\n");
                        dump_msg_content(lo_prio_msg);
                        dump_msg_content(hi_prio_msg);
                        abort();
                    }
                    if(lo_prio_msg->mark != hi_prio_msg->mark) {    //SAME RECEIVERS BUT DIFFERENT MARKS
                        fprintf(stderr, "\tWARNING: same receiver but BUBBLE/NOTICE priority INVERSION\n");
                        dump_msg_content(lo_prio_msg);
                        dump_msg_content(hi_prio_msg);
                        abort();
                    }
                    else {      //BUBBLE MATCHED
                        pack_msg(&rb_ack, lo_prio_msg->receiver, lo_prio_msg->receiver, ASYM_ROLLBACK_ACK,
                                 lo_prio_msg->timestamp, lo_prio_msg->timestamp, 0, NULL);
                        rb_ack->message_kind = control;
                        rb_ack->mark = hi_prio_msg->mark;
                        pt_put_out_msg(rb_ack);
                        msg_release(lo_prio_msg);   //FREE THE BUBBLE
                        msg_release(hi_prio_msg);   //FREE THE NOTICE
                        return;
                    }
                }
            }
            else {  //NOT A CONTROL MESSAGE

                fprintf(stderr, "\tNON-CONTROL msg in hi_prio channel\n");
                dump_msg_content(lo_prio_msg);
                dump_msg_content(hi_prio_msg);
                fflush(stdout);
                abort();
        }
    }
}

/**
<<<<<<< HEAD
* This is a new and simplified version of the asymmetric scheduler. This function extracts a bunch of events
* to be processed by LPs bound to a controller and sends them to processing
* threads for later execution. Rollbacks are executed by the controller, and
* are triggered here in a lazy fashion.
*/
void asym_process(void) {

    msg_t *lo_prio_msg;
    msg_t *hi_prio_msg;
    msg_t *rb_ack;
    int type;

    while((hi_prio_msg = pt_get_hi_prio_msg()) !=  NULL) {
        validate_msg(hi_prio_msg);

        do {
            while ((lo_prio_msg = pt_get_lo_prio_msg()) == NULL);
            validate_msg(lo_prio_msg);

            type = lo_prio_msg->type;
            if (is_control_msg(type)){
                if(type != ASYM_ROLLBACK_BUBBLE){
                    fprintf(stderr, "\tERROR: Type %d CONTROL message SHOULDN'T stay in the LO_PRIO queue!\n",
                            lo_prio_msg->type);
                    dump_msg_content(lo_prio_msg);
                    fflush(stdout);
                    abort();
                }
                else{  //IT IS A BUBBLE
                    if(lo_prio_msg->receiver.to_int != hi_prio_msg->receiver.to_int){   //DIFFERENT RECEIVERS
                        printf("\tERROR: lo/hi prio messages have DIFFERENT receivers\n");
                        dump_msg_content(lo_prio_msg);
                        dump_msg_content(hi_prio_msg);
                        abort();
                    }
                    if(lo_prio_msg->mark != hi_prio_msg->mark) {    //SAME RECEIVERS BUT DIFFERENT MARKS
                        fprintf(stderr, "\tWARNING: same receiver but BUBBLE/NOTICE priority INVERSION\n");
                        dump_msg_content(lo_prio_msg);
                        dump_msg_content(hi_prio_msg);
                        fflush(stdout);
                        abort();
                    }
                    else {  //BUBBLE-NOTICE MATCHED!
                        pack_msg(&rb_ack, lo_prio_msg->receiver, lo_prio_msg->receiver, ASYM_ROLLBACK_ACK,
                                lo_prio_msg->timestamp, lo_prio_msg->timestamp, 0, NULL);
                        rb_ack->message_kind = control;
                        rb_ack->mark = hi_prio_msg->mark;
                        debug("Message ROLLBACK ACK SENT -> LP%u, ts %f\n", lo_prio_msg->receiver.to_int,
                                lo_prio_msg->timestamp);
                        pt_put_out_msg(rb_ack);
                        msg_release(lo_prio_msg);   //FREE THE BUBBLE
                        msg_release(hi_prio_msg);   //FREE THE NOTICE
                        return;
                    }
                }
            }
            else {  //NOT A CONTROL MESSAGE
                if (lo_prio_msg->receiver.to_int != hi_prio_msg->receiver.to_int || lo_prio_msg->timestamp < hi_prio_msg->timestamp ) {
                        asym_process_one_event(lo_prio_msg);
                        continue;
                }
                else {  ///TO BE DISCARDED (ts>bubble_ts) >>>CONTROLLARE<<<
                    lo_prio_msg->unprocessed = false;
                }
            }
        } while (true);
    }

    lo_prio_msg = pt_get_lo_prio_msg();

    if(lo_prio_msg == NULL)
         return;

    type = lo_prio_msg->type;

    if(is_control_msg(lo_prio_msg->type)){

        if(type == ASYM_ROLLBACK_BUBBLE){
            find_a_match(lo_prio_msg);
            return;
        }

        else if (type == ASYM_ROLLBACK_NOTICE) {
            printf("\tERROR: I've found a NOTICE in a lo_prio channel\n");
            dump_msg_content((lo_prio_msg));
            abort();
        }
    }
    asym_process_one_event(lo_prio_msg);
}


void asym_schedule(void) {
    unsigned int i;
    int EventsToAdd = 0;
    int delta_utilization = 0;
    int sent_events = 0;
    unsigned int port_current_size[n_cores];
    unsigned int events_to_fill_PT_port[n_cores];
    unsigned int tot_events_to_schedule = 0;
    unsigned int n_PTs = Threads[tid]->num_PTs;  //PTs assigned to THIS CT
    unsigned long long mark;
    struct lp_struct *chosen_LP;
    msg_t *chosen_EVT;
    msg_t *rb_management;
    msg_t *evt_to_prune, *evt_to_prune_next;

    //timer_start(timer_local_thread);

    for (i = 0; i < n_PTs; i++) {
        Thread_State *PT = Threads[tid]->PTs[i];
        port_current_size[PT->tid] = get_port_current_size(PT->input_port[PORT_PRIO_LO]);
        delta_utilization = PT->port_batch_size - port_current_size[PT->tid];
        if (delta_utilization < 0) { delta_utilization = 0; }
        double utilization_rate = 1.0 - ((double) delta_utilization / (double) PT->port_batch_size);

        ///The bigger the utilization rate is, the smaller amount of free space the port can offer
        //   printf("port_current_size[PT->tid]: %d, utilization_rate: %f, port_batch_size: %d \n",port_current_size[PT->tid], utilization_rate,PT->port_batch_size);
        if (utilization_rate > UPPER_PORT_THRESHOLD) {
            modify_score(UPPER_THRESHOLD_MODIFIER);
            if (PT->port_batch_size <= (MAX_PORT_SIZE - BATCH_STEP)) {
                PT->port_batch_size += BATCH_STEP;
            } else if (PT->port_batch_size < MAX_PORT_SIZE) {
                PT->port_batch_size++;
            }
        } else if (utilization_rate < LOWER_PORT_THRESHOLD) {
            modify_score(LOWER_THRESHOLD_MODIFIER);
            if (PT->port_batch_size > BATCH_STEP) {
                PT->port_batch_size -= BATCH_STEP;
            } else if (PT->port_batch_size > 1) {
                PT->port_batch_size--;
            }
        }

        EventsToAdd = PT->port_batch_size - port_current_size[PT->tid];
        if (EventsToAdd > 0) {
            events_to_fill_PT_port[PT->tid] = EventsToAdd;
            tot_events_to_schedule += EventsToAdd;
        } else {
            events_to_fill_PT_port[PT->tid] = 0;
        }
    }

    memcpy(asym_lps_mask, lps_bound_blocks, sizeof(struct lp_struct *) * n_lp_per_thread);
    for (i = 0; i < n_lp_per_thread; i++) {
        Thread_State *PT = Threads[asym_lps_mask[i]->processing_thread];  //PT assigned to that lp "i"
        if (port_current_size[PT->tid] >= PT->port_batch_size) {
            asym_lps_mask[i] = NULL;
        }
    }

    // Pointer to an array of chars used by controllers as a counter of the number of events scheduled for
    // each LP during the execution of asym_schedule.
    bzero(Threads[tid]->curr_scheduled_events, sizeof(int) * n_prc);

    for (i = 0; i < tot_events_to_schedule; i++) {
        if (rootsim_config.scheduler == SCHEDULER_STF) {
            chosen_LP = asym_smallest_timestamp_first();
        } else {
            fprintf(stderr, "\tWARNING: asym scheduler supports only the STF scheduler by now\n");
            abort();
        }

        if (unlikely(chosen_LP == NULL)) {
            //  statistics_post_data(NULL, STAT_IDLE_CYCLES, 1.0);
            return;
        }

        if (chosen_LP->state == LP_STATE_ROLLBACK) { // = LP received an out-of-order msg and needs a rollback
            mark = generate_mark(chosen_LP);

            if (chosen_LP->rollback_status == REQUESTED)
                chosen_LP->rollback_status = PROCESSING;

            else {
                printf("\tERROR: Impossible rollback_status\n");
                abort();
            }

            chosen_LP->rollback_mark = mark;

            pack_msg(&rb_management, chosen_LP->gid, chosen_LP->gid, ASYM_ROLLBACK_NOTICE, chosen_LP->bound->timestamp,
                     chosen_LP->bound->timestamp, 0, NULL);// Send rollback notice in the high priority port
            rb_management->message_kind = control;
            rb_management->mark = mark;
            chosen_LP->start = clock();   ///Start the turnaround timer
            pt_put_hi_prio_msg(chosen_LP->processing_thread, rb_management);

            chosen_LP->state = LP_STATE_WAIT_FOR_ROLLBACK_ACK;  //BLOCKED STATE

            pack_msg(&rb_management, chosen_LP->gid, chosen_LP->gid, ASYM_ROLLBACK_BUBBLE, chosen_LP->bound->timestamp,
                     chosen_LP->bound->timestamp, 0, NULL);
            rb_management->message_kind = control;
            rb_management->mark = mark;
            pt_put_lo_prio_msg(chosen_LP->processing_thread, rb_management);

            continue;
        }

        if (chosen_LP->state == LP_STATE_ROLLBACK_ALLOWED) { // = extracted ASYM_ROLLBACK_ACK from PT output queue for chosen_LP
            chosen_LP->state = LP_STATE_ROLLBACK;
            rollback(chosen_LP);
            chosen_LP->state = LP_STATE_READY;

            evt_to_prune = list_head(chosen_LP->retirement_queue);
            while (evt_to_prune != NULL) {
                evt_to_prune_next = list_next(evt_to_prune);
                if (evt_to_prune->unprocessed == false && chosen_LP->last_processed != evt_to_prune) {
                    list_delete_by_content(chosen_LP->retirement_queue, evt_to_prune);
                    msg_release(evt_to_prune);
                }
                evt_to_prune = evt_to_prune_next;
            }
        }

        if (chosen_LP->state != LP_STATE_READY_FOR_SYNCH && !is_blocked_state(chosen_LP->state)) {
            chosen_EVT = advance_to_next_event(chosen_LP);
        } else {
            chosen_EVT = chosen_LP->bound;
        }

        if (unlikely(chosen_EVT == NULL)) {
            rootsim_error(true,"Critical condition: LP %d seems to have events to be processed, but I cannot find them. Aborting...\n", chosen_LP->gid);
        }

        if (unlikely(!to_be_sent_to_LP(chosen_EVT))) {    //NOT a message to be passed to the LP (a control msg)
            return;
        }
        if(are_input_channels_empty(chosen_LP->processing_thread)){
           exponential_moving_avg(1, EMPTY_PT_ID);
        }
        else
           exponential_moving_avg(0,EMPTY_PT_ID);

        chosen_EVT->unprocessed = true;
        pt_put_lo_prio_msg(chosen_LP->processing_thread, chosen_EVT);
        sent_events++;
        events_to_fill_PT_port[chosen_LP->processing_thread]--;
        unsigned int chosen_LP_id = chosen_LP->lid.to_int;

        if (rootsim_config.scheduler == SCHEDULER_STF) {

            Threads[tid]->curr_scheduled_events[chosen_LP_id] = Threads[tid]->curr_scheduled_events[chosen_LP_id] + 1;

            if (Threads[tid]->curr_scheduled_events[chosen_LP_id] >= MAX_LP_EVENTS_PER_BATCH) {
                //FIND THE LP IN THE MASK AND SET IT TO NULL
                for (i = 0; i < n_lp_per_thread; i++) {
                    if (asym_lps_mask[i] != NULL && lid_equals(asym_lps_mask[i]->lid, chosen_LP->lid)) {
                        asym_lps_mask[i] = NULL;
                        break;
                    }
                }
            }

            if (events_to_fill_PT_port[chosen_LP->processing_thread] == 0) {  //NO MORE EMPTY SLOTS FOR THAT PT
                //FIND THE LP IN THE MASK AND SET IT TO NULL
                for (i = 0; i < n_lp_per_thread; i++) {
                    if (asym_lps_mask[i] != NULL && asym_lps_mask[i]->processing_thread == chosen_LP->processing_thread)
                        asym_lps_mask[i] = NULL;
                }
            }
        }
    }

    if (sent_events == 0) {
            //  total_idle_microseconds[tid] += timer_value_micro(timer_local_thread);
        }
}


/**
* This function checks which LP must be activated (if any),
=======
 * This is the core processing routine of PTs
 *
 * @Author: Stefano Conoci
 * @Author: Alessandro Pellegrini
 */
void asym_process(void) {
	msg_t *msg;
	msg_t *msg_hi;
	msg_t *hi_prio_msg;
	LID_t lid;

	timer_start(timer_local_thread);

	// We initially check for high priority msgs. If one is present,
	// we process it and then return. In this way, the next call to
	// asym_process() will again check for high priority events, making
	// them really high priority.
	msg = pt_get_hi_prio_msg();
	if(msg != NULL) {
		list_insert_tail(hi_prio_list, msg);
		// Never change this return to anything else: we call asym_process()
		// within asym_process() to forcely match a high priority queue when
		// there could be a priority inversion between hi and lo prio ports.
		return;
	}

	// No high priority message. Get an event to process.
	msg = pt_get_lo_prio_msg();

	// My queue might be empty...
	if(msg == NULL){
		//printf("Empty queue for PT %d\n", tid);
		total_idle_microseconds[tid] += timer_value_micro(timer_local_thread);
		return;
	}

	// Check if we picked a stale message
	hi_prio_msg = list_head(hi_prio_list);
	while(hi_prio_msg != NULL) {
		if(gid_equals(msg->receiver, hi_prio_msg->receiver) && !is_control_msg(msg->type) && msg->timestamp > hi_prio_msg->timestamp) {
			// TODO: switch to bool
			if(hi_prio_msg->event_content[0] == 0) {
				hi_prio_msg->event_content[0] = 1;

//				printf("Sending a ROLLBACK_ACK for LP %d at time %f\n", msg->receiver.id, msg->timestamp);
				// atomic_sub(&notice_count, 1);

				// Send back an ack to start processing the actual rollback operation
				pack_msg(&msg, msg->receiver, msg->receiver, ASYM_ROLLBACK_ACK, msg->timestamp, msg->timestamp, 0, NULL);
				msg->message_kind = control;
				pt_put_out_msg(msg);
			}
			return;
		}
		hi_prio_msg = list_next(hi_prio_msg);
	}

	// Match a ROLLBACK_NOTICE with a ROLLBACK_BUBBLE and remove it from the
	// local hi_priority_list.
	if(is_control_msg(msg->type) && msg->type == ASYM_ROLLBACK_BUBBLE) {
//		asym_process();

           ariprova:
		hi_prio_msg = list_head(hi_prio_list);
		while(hi_prio_msg != NULL) {
			if(msg->mark == hi_prio_msg->mark) {
//				msg_release(msg);

				// TODO: switch to bool
				if(hi_prio_msg->event_content[0] == 0) {
//g					printf("Sending a ROLLBACK_ACK (2)  for LP %d at time %f\n", msg->receiver.id, msg->timestamp);
					// atomic_sub(&notice_count, 1);

					// Send back an ack to start processing the actual rollback operation
					pack_msg(&msg, msg->receiver, msg->receiver, ASYM_ROLLBACK_ACK, msg->timestamp, msg->timestamp, 0, NULL);
					msg->message_kind = control;
					pt_put_out_msg(msg);
				}

				list_delete_by_content(hi_prio_list, hi_prio_msg);
//				msg_release(hi_prio_msg);
				return;
			}
			hi_prio_msg = list_next(hi_prio_msg);
		}

		msg_hi = pt_get_hi_prio_msg();
		while(msg_hi != NULL) {
			list_insert_tail(hi_prio_list, msg_hi);
			msg_hi = pt_get_hi_prio_msg();
		}
		goto ariprova;

		fprintf(stderr, "Cannot match a bubble!\n");
		abort();
	}

	lid = GidToLid(msg->receiver);

	// TODO: find a way to set the LP to RUNNING without incurring in a race condition with the CT

	// Process this event
	activate_LP(lid, msg->timestamp, msg, LPS(lid)->current_base_pointer);
	my_processed_events++;
	msg->unprocessed = false;

	// Send back to the controller the (possibly) generated events
	asym_send_outgoing_msgs(lid);

	// Log the state, if needed
	// TODO: we make the PT take a checkpoint. The optimality would be to let the CT
	// take a checkpoint, but we need some sort of synchronization which is out of the
	// scope of the current development phase here.
	LogState(lid);
	
	//printf("asym_process for PT %d completed in %d millisecond\n", tid, timer_value_milli(timer_local_thread));

}


/**
* This is the asymmetric scheduler. This function extracts a bunch of events
* to be processed by LPs bound to a controller and sends them to processing
* threads for later execution. Rollbacks are executed by the controller, and
* are triggered here in a lazy fashion.
*
* @Author Stefano Conoci
* @Author Alessandro Pellegrini
*/
void asym_schedule(void) {
	LID_t lid, curr_lid;
	msg_t *event, *curr_event;
	msg_t *rollback_control;
	unsigned int port_events_to_fill[n_cores];
	unsigned int port_current_size[n_cores];
	unsigned int i, j, thread_id_mask;
	unsigned int events_to_fill = 0;
	char first_encountered = 0;
	unsigned long long mark;
	int toAdd, delta_utilization;
	unsigned int sent_events = 0; 
	unsigned int sent_notice = 0;

	/* Testing heap 
	heap_t heap_test; 
	heap_test.size = 0;
	heap_test.len = 0;
	
	if(lps_blocks[1]->bound != NULL && lps_blocks[2]->bound != NULL){
		// DEBUG: testing heap implementantion
		printf("Adding to heap bound of LP 1 and LP2\n");
		heap_push(&heap_test, lps_blocks[1]->bound->timestamp, lps_blocks[1]->bound);
		heap_push(&heap_test, lps_blocks[2]->bound->timestamp, lps_blocks[2]->bound);
		printf("Retrieving LPS in order of timestamp bound\n");
		msg_t *first_dequeue = heap_pop(&heap_test);
		msg_t *second_dequeue = heap_pop(&heap_test);
		printf("First dequeue: %lf, second dequeue: %lf\n", first_dequeue->timestamp, second_dequeue->timestamp);
	}
	*/

//	printf("NOTICE COUNT: %d\n", atomic_read(&notice_count));

	timer_start(timer_local_thread);

	// Compute utilization rate of the input ports since the last call to asym_schedule
	// and resize the ports if necessary
	for(i = 0; i < Threads[tid]->num_PTs; i++){
		Thread_State* pt = Threads[tid]->PTs[i];
		int port_size = pt->port_batch_size; 
		port_current_size[pt->tid] = get_port_current_size(pt->input_port[PORT_PRIO_LO]);
		delta_utilization = port_size - port_current_size[pt->tid];
		if(delta_utilization < 0){
			delta_utilization = 0;
		}
		double utilization_rate = (double) delta_utilization / (double) port_size;
	
		// DEBUG
		//printf("Port current size: %u, port size %u, delta_utilization %d\n", port_current_size[pt->tid], port_size, delta_utilization); 
		//printf("Input port size of PT %u: %d (utilization factor: %f)\n", pt->tid, port_current_size[pt->tid], utilization_rate);
		// DEBUG

		// If utilization rate is too high, the size of the port should be increased
		if(utilization_rate > UPPER_PORT_THRESHOLD){
			if(pt->port_batch_size <= (MAX_PORT_SIZE - BATCH_STEP)){
				pt->port_batch_size+=BATCH_STEP;
			}else if(pt->port_batch_size < MAX_PORT_SIZE){
				pt->port_batch_size++;
			}
			//printf("Increased port size of PT %u to %d\n", pt->tid, pt->port_batch_size);
		}
		// If utilization rate is too low, the size of the port should be decreased
		else if (utilization_rate < LOWER_PORT_THRESHOLD){
			if(pt->port_batch_size > BATCH_STEP){
				pt->port_batch_size-=BATCH_STEP;
			}else if(pt->port_batch_size > 1){
				pt->port_batch_size--;
			}
			//printf("Reduced port size of PT %u to %d\n", pt->tid, pt->port_batch_size);
		}
	}
	
	//printf("asym_schedule: port resize completed for controller %d at millisecond %d\n", tid, timer_value_milli(timer_local_thread));


	// Compute the total number of events necessary to fill all
	// the input ports, considering the current batch value 
	// and the current number of events yet to be processed in 
	// each port
	for(i = 0; i < Threads[tid]->num_PTs; i++){
		Thread_State* pt = Threads[tid]->PTs[i];
		toAdd = pt->port_batch_size - port_current_size[pt->tid];
		// Might be negative as we reduced the port size, and it might 
		// have been already full
		if(toAdd > 0){	
			port_events_to_fill[pt->tid] = toAdd;
			events_to_fill += toAdd;
			//printf("Adding toAdd = %d to events_to_fill\n", toAdd);
		} else {
			port_events_to_fill[pt->tid] = 0;
		}
	}

	//printf("asym_schedule: events_to_fill computed for controller %d at millisecond %d\n", tid, timer_value_milli(timer_local_thread));


	// Create a copy of lps_bound_blocks in asym_lps_mask which will
	// be modified during scheduling in order to jump LPs bound to PT
	// for whom the input port is already filled
	memcpy(asym_lps_mask, lps_bound_blocks, sizeof(LP_State *) * n_prc_per_thread);
	for(i = 0; i < n_prc_per_thread; i++) {
		Thread_State *pt = Threads[asym_lps_mask[i]->processing_thread];
		if(port_current_size[pt->tid] >= pt->port_batch_size) {
			asym_lps_mask[i] = NULL;
		}
	}
	//printf("asym_schedule: asym_lps_mask computed for controller %d at millisecond %d\n", tid, timer_value_milli(timer_local_thread));

	// Put up to MAX_LP_EVENTS_PER_BATCH events for each LP in the priority
	// queue events_heap
	if(rootsim_config.scheduler == BATCH_LOWEST_TIMESTAMP){
		// Clean the priority queue
		bzero(Threads[tid]->events_heap->nodes, Threads[tid]->events_heap->len*sizeof(node_heap_t));
		Threads[tid]->events_heap->len = 0;

		for(i = 0; i < n_prc_per_thread; i++){
			if(asym_lps_mask[i] != NULL && !is_blocked_state(asym_lps_mask[i]->state)){
				if(asym_lps_mask[i]->bound == NULL && !list_empty(asym_lps_mask[i]->queue_in)){
					curr_event = list_head(asym_lps_mask[i]->queue_in);
				}
				else{
					curr_event = list_next(asym_lps_mask[i]->bound);
				}

				j = 0;
				while(curr_event != NULL && j < MAX_LP_EVENTS_PER_BATCH){
					if(curr_event->timestamp >= 0){
						heap_push(Threads[tid]->events_heap, curr_event->timestamp, curr_event);
						//printf("Pushing to priority queue event from LP %d with timestamp %lf\n", 
								 //lid_to_int(asym_lps_mask[i]->lid), curr_event->timestamp);
						j++;
						curr_event = curr_event->next;
					}
				}
			}
		}
	}

	// Set to 0 all the curr_scheduled_events array
	bzero(Threads[tid]->curr_scheduled_events, sizeof(int)*n_prc);

	//printf("events_to_fill=%d\n", events_to_fill);

	for(i = 0; i < events_to_fill; i++) {

//		printf("SCHED: mask: ");
//		int jk;
//		for(jk = 0; jk < n_prc_per_thread; jk++) {
//			printf("%p, ", asym_lps_mask[jk]);
//		}
//		puts("");

		#ifdef HAVE_CROSS_STATE
		bool resume_execution = false;
		#endif


		// Find next LP to be executed, depending on the chosen scheduler
		switch (rootsim_config.scheduler) {

			case SMALLEST_TIMESTAMP_FIRST:
				lid = asym_smallest_timestamp_first();
				break;

			case BATCH_LOWEST_TIMESTAMP:
				curr_event = heap_pop(Threads[tid]->events_heap);
				//printf("Retrieving from priority queue event from LP %d with timestamp %lf\n",
				//		lid_to_int(GidToLid(curr_event->sender)), curr_event->timestamp);
				int found = 0;
			       	lid = idle_process;	
				while(curr_event != NULL && !found){
					curr_lid = GidToLid(curr_event->receiver);
					if(port_events_to_fill[LPS(curr_lid)->processing_thread] > 0 &&
					  LPS(curr_lid)->state != LP_STATE_WAIT_FOR_ROLLBACK_ACK){
						found = 1; 
						lid = curr_lid;
					}
					else{
						curr_event = heap_pop(Threads[tid]->events_heap);
						lid = idle_process;
					}
				}
				break;
	
			default:
				lid = asym_smallest_timestamp_first();
		}

		// No logical process found with events to be processed
		if (lid_equals(lid, idle_process)) {
			statistics_post_lp_data(lid, STAT_IDLE_CYCLES, 1.0);
			continue;
		}

		// If we have to rollback
		if(LPS(lid)->state == LP_STATE_ROLLBACK) {
			// Get a mark for the current batch of control messages
			mark = generate_mark(lid);

			//printf("Sending a ROLLBACK_NOTICE for LP %d at time %f\n", lid.id, lvt(lid));
			sent_notice++;

			// atomic_add(&notice_count, 1);

			// Send rollback notice in the high priority port
			pack_msg(&rollback_control, LidToGid(lid), LidToGid(lid), ASYM_ROLLBACK_NOTICE, lvt(lid), lvt(lid), sizeof(char), &first_encountered);
			rollback_control->message_kind = control;
			rollback_control->mark = mark;
			pt_put_hi_prio_msg(LPS(lid)->processing_thread, rollback_control);

			// Set the LP to a blocked state
			LPS(lid)->state = LP_STATE_WAIT_FOR_ROLLBACK_ACK;

			// Notify the PT in charge of managing this LP that the rollback is complete and
			// events to the LP should not be discarded anymore
//			printf("Sending a ROLLBACK_BUBBLE for LP %d at time %f\n", lid.id, lvt(lid));

			pack_msg(&rollback_control, LidToGid(lid), LidToGid(lid), ASYM_ROLLBACK_BUBBLE, lvt(lid), lvt(lid), 0, NULL);
			rollback_control->message_kind = control;
			rollback_control->mark = mark;
			pt_put_lo_prio_msg(LPS(lid)->processing_thread, rollback_control);

			continue;
		}

		if(LPS(lid)->state == LP_STATE_ROLLBACK_ALLOWED) {
			// Rollback the LP and send antimessages
			LPS(lid)->state = LP_STATE_ROLLBACK;
			rollback(lid);
			LPS(lid)->state = LP_STATE_READY;
			//send_outgoing_msgs(lid);

			// Prune the retirement queue for this LP
			while(true) {
				event = list_head(LPS(lid)->retirement_queue);
				if(event == NULL) {
					break;
				}
				list_delete_by_content(LPS(lid)->retirement_queue, event);
				msg_release(event);
			}

			continue;
		}

		if(!is_blocked_state(LPS(lid)->state) && LPS(lid)->state != LP_STATE_READY_FOR_SYNCH){
			event = advance_to_next_event(lid);
		} else {
			event = LPS(lid)->bound;
		}


		// Sanity check: if we get here, it means that lid is a LP which has
		// at least one event to be executed. If advance_to_next_event() returns
		// NULL, it means that lid has no events to be executed. This is
		// a critical condition and we abort.
		if(event == NULL) {
			rootsim_error(true, "Critical condition: LP %d seems to have events to be processed, but I cannot find them. Aborting...\n", lid);
		}

		if(!process_control_msg(event)) {
			return;
		}

		#ifdef HAVE_CROSS_STATE
		// TODO: we should change this by managing the state internally to activate_LP, as this
		// would uniform the code across symmetric/asymmetric implementations.
		// In case we are resuming an interrupted execution, we keep track of this.
		// If at the end of the scheduling the LP is not blocked, we can unblock all the remote objects
		if(is_blocked_state(LPS(lid)->state) || LPS(lid)->state == LP_STATE_READY_FOR_SYNCH) {
			resume_execution = true;
		}
		#endif

		thread_id_mask = LPS(lid)->processing_thread;

		// Put the event in the low prio queue of the associated PT
		event->unprocessed = true;
		pt_put_lo_prio_msg(thread_id_mask, event);
		sent_events++;

		// Modify port_events_to_fill to reflect last message sent
		port_events_to_fill[thread_id_mask]--; 

		unsigned int lp_id = lid_to_int(lid);
		
		if(rootsim_config.scheduler == SMALLEST_TIMESTAMP_FIRST){
			// Increase curr_scheduled_events, and set pointer to 
			// respective LP to NULL if exceeded MAX_LP_EVENTS_PER_BATCH
			Threads[tid]->curr_scheduled_events[lp_id] = Threads[tid]->curr_scheduled_events[lp_id]+1;
			//printf("curr_scheduled_events[%u] = %d\n", lp_id, Threads[tid]->curr_scheduled_events[lp_id]);
			if(Threads[tid]->curr_scheduled_events[lp_id] >= MAX_LP_EVENTS_PER_BATCH){
				for(i=0; i<n_prc_per_thread; i++){
					if(asym_lps_mask[i] != NULL && lid_equals(asym_lps_mask[i]->lid,lid)){
						asym_lps_mask[i] = NULL;
						//printf("Setting to NULL pointer to LP %d\n", lp_id);
						break;
					}
				}
			}

			//printf("asym_schedule: event sent for controller %d at millisecond %d\n", tid, timer_value_milli(timer_local_thread));

			// If one port becomes full, should set all pointers to LP
			// mapped to the PT of the respective port to NULL 
			// printf("thread_id_mask: %u\n", thread_id_mask);
			if(port_events_to_fill[thread_id_mask] == 0){
				for(i = 0; i<n_prc_per_thread; i++){
					if(asym_lps_mask[i] != NULL && asym_lps_mask[i]->processing_thread == thread_id_mask)
						asym_lps_mask[i] = NULL;
				}
			}

		}
		
		#ifdef HAVE_CROSS_STATE
		if(resume_execution && !is_blocked_state(LPS(lid)->state)) {
			printf("ECS event is finished mark %llu !!!\n", LPS(lid)->wait_on_rendezvous);
			fflush(stdout);
			unblock_synchronized_objects(lid);

			// This is to avoid domino effect when relying on rendezvous messages
			// TODO: I'm not quite sure if with asynchronous PTs' this way to code ECS-related checkpoints still holds
			force_LP_checkpoint(lid);
		}
		#endif
	}

	if(sent_events == 0){
		total_idle_microseconds[tid] += timer_value_micro(timer_local_thread);
	}

	//printf("Sent events: %u\n", sent_events);
	//printf("Sent rollback notice: %u\n", sent_notice);
	//printf("asym_schedule for controller %d completed in %d milliseconds\n", tid, timer_value_milli(timer_local_thread));
}

/**
* This function checks wihch LP must be activated (if any),
>>>>>>> origin/power
* and in turn activates it. This is used only to support forward execution.
*
* @author Alessandro Pellegrini
*/
void schedule(void)
{
	struct lp_struct *next = NULL;
	msg_t *event;

#ifdef HAVE_CROSS_STATE
	bool resume_execution = false;
#endif

	// Find the next LP to be scheduled
	switch (rootsim_config.scheduler) {

		case SCHEDULER_STF:
			next = smallest_timestamp_first();
			break;

		default:
			rootsim_error(true, "unrecognized scheduler!");
	}

	// No logical process found with events to be processed
<<<<<<< HEAD
	if (next == NULL) {
		statistics_post_data(NULL, STAT_IDLE_CYCLES, 1.0);
		return;
	}

	// If we have to rollback
	if (next->state == LP_STATE_ROLLBACK) {
		rollback(next);
		next->state = LP_STATE_READY;
		send_outgoing_msgs(next);
=======
	if (lid == IDLE_PROCESS) {
		statistics_post_lp_data(lid, STAT_IDLE_CYCLES, 1.0);
      	return;
    }

	// If we have to rollback
    if(LPS[lid]->state == LP_STATE_ROLLBACK) {

		if (has_cancelback_started()) {
			// stylized_printf("LP cannot rollback in order to sync for Cancelback!\n", CYAN, true);
		} else {
			rollback(lid);

			// Discard any possible execution state related to a blocked execution
			#ifdef ENABLE_ULT
			memcpy(&LPS[lid]->context, &LPS[lid]->default_context, sizeof(LP_context_t));
			#endif

			LPS[lid]->state = LP_STATE_READY;
			send_outgoing_msgs(lid);
		}

<<<<<<< HEAD
=======
		LPS[lid]->state = LP_STATE_READY;
		send_outgoing_msgs(lid);
		process_bottom_halves();
>>>>>>> origin/reverse
		return;

	} else if (LPS[lid]->state == LP_STATE_CANCELBACK) {

        send_cancelback_messages(lid);

		// LPS[lid]->state = LP_STATE_READY;
		return;
	
	} else if (LPS[lid]->state == LP_STATE_SYNCH_FOR_CANCELBACK) {
		
		LPS[lid]->state = LPS[lid]->state_to_resume;
		LPS[lid]->state_to_resume = 0;

		if (LPS[lid]->state != LP_STATE_READY)
			log_state_switch(lid);

>>>>>>> origin/cancelback
		return;
	}

	if (!is_blocked_state(next->state)
	    && next->state != LP_STATE_READY_FOR_SYNCH) {
		event = advance_to_next_event(next);
	} else {
		event = next->bound;
	}

	// Sanity check: if we get here, it means that lid is a LP which has
	// at least one event to be executed. If advance_to_next_event() returns
	// NULL, it means that lid has no events to be executed. This is
	// a critical condition and we abort.
	if (unlikely(event == NULL)) {
		rootsim_error(true,
			      "Critical condition: LP %d seems to have events to be processed, but I cannot find them. Aborting...\n",
			      next->gid);
	}

	if (unlikely(!to_be_sent_to_LP(event))) {
		return;
	}
#ifdef HAVE_CROSS_STATE
	// In case we are resuming an interrupted execution, we keep track of this.
	// If at the end of the scheduling the LP is not blocked, we can unblock all the remote objects
	if (is_blocked_state(next->state) || next->state == LP_STATE_READY_FOR_SYNCH) {
		resume_execution = true;
	}
<<<<<<< HEAD
#endif
=======
	#endif
>>>>>>> origin/cancelback

	// Schedule the LP user-level thread
	if (next->state == LP_STATE_READY_FOR_SYNCH)
		next->state = LP_STATE_RUNNING_ECS;
	else
		next->state = LP_STATE_RUNNING;
	activate_LP(next, event);

	if (!is_blocked_state(next->state)) {
		next->state = LP_STATE_READY;
		send_outgoing_msgs(next);
	}
#ifdef HAVE_CROSS_STATE
	if (resume_execution && !is_blocked_state(next->state)) {
		//printf("ECS event is finished mark %llu !!!\n", next->wait_on_rendezvous);
		fflush(stdout);
		unblock_synchronized_objects(next);

		// This is to avoid domino effect when relying on rendezvous messages
		force_LP_checkpoint(next);
	}
#endif

	// Accurate termination detection
	ccgs_lp_can_halt(next);

	// Log the state, if needed
	LogState(next);
}

void schedule_on_init(struct lp_struct *lp)
{
	msg_t *event;

	event = list_head(lp->queue_in);
	lp->bound = event;

	// Sanity check: if we get here, it means that lid is a LP which has
	// at least one event to be executed. If advance_to_next_event() returns
	// NULL, it means that lid has no events to be executed. This is
	// a critical condition and we abort.
	if (unlikely(event == NULL) || event->type != INIT) {
		rootsim_error(true,
			      "Critical condition: LP %d should have an INIT event but I cannot find it. Aborting...\n",
			      lp->gid.to_int);
	}

	lp->state = LP_STATE_RUNNING;

<<<<<<< HEAD
	activate_LP(next, event);
	if(rootsim_config.num_controllers>0)
	    next->last_processed = next->next_last_processed;

	if (!is_blocked_state(next->state)) {
		next->state = LP_STATE_READY;
		send_outgoing_msgs(next);
	}
#ifdef HAVE_CROSS_STATE
<<<<<<< HEAD
	if (resume_execution && !is_blocked_state(next->state)) {
		//printf("ECS event is finished mark %llu !!!\n", next->wait_on_rendezvous);
		fflush(stdout);
		unblock_synchronized_objects(next);

=======
	if(resume_execution && !is_blocked_state(LPS(lid)->state)) {
		//printf("ECS event is finished at LP %d mark %llu !!!\n", lid_to_int(lid), LPS(lid)->wait_on_rendezvous);
		fflush(stdout);
		unblock_synchronized_objects(lid);
		statistics_post_lp_data(lid, STAT_ECS, 1.0);
>>>>>>> origin/ecs
		// This is to avoid domino effect when relying on rendezvous messages
		force_LP_checkpoint(next);
	}
#endif
=======
	current = lp;
	current_evt = event;
	ProcessEvent(current->gid.to_int, event->timestamp, event->type,
		     event->event_content, event->size, current->current_base_pointer);
	current_evt = NULL;
	current = NULL;

	lp->state = LP_STATE_READY;
	send_outgoing_msgs(lp);
>>>>>>> origin/incremental

	// If we run using incremental state saving, the very first snapshot must
	// be anyhow a full checkpoint
	set_force_full(lp);
	force_LP_checkpoint(lp);
	// Log the state, if needed
	LogState(lp);
}
