/**
* @file gvt/gvt.c
*
* @brief Global Virtual Time
*
* This module implements the GVT reduction. The current implementation
* is non blocking for observable simulation plaftorms.
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
* @author Francesco Quaglia
* @author Tommaso Tocci
*
* @date June 14, 2014
*/

#include <ROOT-Sim.h>
#include <arch/thread.h>
<<<<<<< HEAD
#include <limits.h>
=======
#include <arch/atomic.h>
>>>>>>> origin/atomic
#include <gvt/gvt.h>
#include <gvt/ccgs.h>
#include <core/core.h>
#include <core/init.h>
#include <core/timer.h>
<<<<<<< HEAD
#include <scheduler/binding.h>
#include <scheduler/process.h>
#include <scheduler/scheduler.h>
=======
#include <core/power.h>
#include <scheduler/process.h>
#include <scheduler/scheduler.h> // this is for n_prc_per_thread
>>>>>>> origin/power
#include <statistics/statistics.h>
#include <mm/mm.h>
#include <communication/mpi.h>
#include <communication/gvt.h>

<<<<<<< HEAD
/// A constant 1 to be used in CAS operations to set tokens
__thread const atomic_int one = 1;
=======
>>>>>>> origin/power

enum kernel_phases {
	kphase_start,
    #ifdef HAVE_MPI
	kphase_white_msg_redux,
    #endif
	kphase_kvt,
    #ifdef HAVE_MPI
	kphase_gvt_redux,
    #endif
	kphase_fossil,
	kphase_idle
};

enum thread_phases {
	tphase_A,
	tphase_send,
	tphase_B,
	tphase_aware,
	tphase_idle
};

// Timer to know when we have to start GVT computation.
// Each thread could start the GVT reduction phase, so this
// is a per-thread variable.
timer gvt_timer;

timer gvt_round_timer;

#ifdef HAVE_MPI
static atomic_t init_kvt_tkn;
static atomic_t commit_gvt_tkn;
#endif

/* Data shared across threads */

static atomic enum kernel_phases kernel_phase = kphase_idle;

static atomic_t init_completed_tkn;
static atomic_t commit_kvt_tkn;
static atomic_t idle_tkn;

static atomic_t counter_initialized;
static atomic_t counter_kvt;
static atomic_t counter_finalized;

/// To be used with CAS to determine who is starting the next GVT reduction phase
static atomic_t current_GVT_round = 0;

/// How many threads have left phase A?
static atomic_t counter_A;

/// How many threads have left phase send?
static atomic_t counter_send;

/// How many threads have left phase B?
static atomic_t counter_B;

<<<<<<< HEAD
=======
/// How many threads are aware that the GVT reduction is over?
static atomic_t counter_aware;

/// How many threads have acquired the new GVT?
static atomic_t counter_end;

/// Keeps track of Cancelback cooldown
static atomic_t counter_cancelback;

/** Flag to start a new GVT reduction phase. Explicitly using an int here,
 *  because 'bool' could be compiler dependent, but we must know the size
 *  beforehand, because we're going to use CAS on this. Changing the type could
 *  entail an undefined behaviour. 'false' and 'true' are usually int's (or can be
 *  converted to them by the compiler), so everything should work here.
 */
static volatile unsigned int GVT_flag = 0;

/// Pointers to the barrier states of the bound LPs
static state_t **time_barrier_pointer;

/// This is the thread which will be assigned the task to start the Cancelback protocol
static volatile unsigned int cancelback_tid = UINT_MAX;

// Whether Cancelback was started during last iteration
static volatile unsigned int cancelback_flag = 0;

>>>>>>> origin/cancelback
/** Keep track of the last computed gvt value. Its a per-thread variable
 * to avoid synchronization on it, but eventually all threads write here
 * the same exact value.
 * The 'adopted_last_gvt' version is used to maintain the adopted gvt
 * value in a temporary variable. It is then copied to last_gvt during
 * the end phase, to avoid possible critical races when checking the
 * termination upon reaching a certain simulation time value.
 */
static __thread simtime_t last_gvt = 0.0;

<<<<<<< HEAD
static simtime_t updated_gvt = 0.0;

// last agreed KVT
static atomic simtime_t new_gvt = 0.0;

=======
>>>>>>> origin/cancelback
/// What is my phase? All threads start in the initial phase
static __thread enum thread_phases thread_phase = tphase_idle;

/// Per-thread GVT round counter
static __thread atomic_t my_GVT_round = 0;

/// The local (per-thread) minimum. It's not TLS, rather an array, to allow reduction by master thread
static simtime_t *local_min;

static simtime_t *local_min_barrier;

<<<<<<< HEAD
<<<<<<< HEAD
/** The number of threads participating to the GVT computation on a single node depends on whether
 * we are running with the symmetric or asymmetric configuration, and it can potentially change
 * over time. This variable tells how many threads are participating to the GVT.
 */
static unsigned int gvt_participants;
=======
/// Total number of pages available on the system
static size_t tot_pages;
>>>>>>> origin/cancelback
=======
// The number of threads participating to the GVT computation on a single node depends on whether
// we are running with the symmetric or asymmetric configuration, and it can potentially change
// over time. This variable tells how many threads are participating to the GVT.
static unsigned int gvt_participants;
>>>>>>> origin/power

/**
* Initialization of the GVT subsystem.
*/
void gvt_init(void) {

	unsigned int i;

	tot_pages = sysconf(_SC_PHYS_PAGES);
	size_t cb_threshold = (size_t)(tot_pages * CANCELBACK_MEM_THRESHOLD);
	printf("Total number of pages on system: %zu\nThreshold is %zu\n", tot_pages, cb_threshold);
	
	double page_size = sysconf(_SC_PAGESIZE) / 1000.0;
	printf("Page size on this system: %f KB\n", page_size);

	// This allows the first GVT phase to start
	atomic_set(&counter_finalized, 0);

	/// This allows to enable Cancelback trigger condition
	atomic_set(&counter_cancelback, 0);

	// Initialize the local minima
<<<<<<< HEAD
	local_min =         rsalloc(sizeof(simtime_t) * n_cores);
	local_min_barrier = rsalloc(sizeof(simtime_t) * n_cores);
	for (i = 0; i < n_cores; i++) {
=======
	local_min = rsalloc(sizeof(simtime_t) * n_cores);
	local_min_barrier = rsalloc(sizeof(simtime_t) * n_cores);
	for(i = 0; i < n_cores; i++) {
>>>>>>> origin/cancelback
		local_min[i] = INFTY;
		local_min_barrier[i] = INFTY;
	}

<<<<<<< HEAD
    if(rootsim_config.num_controllers > 0) {
        gvt_participants = rootsim_config.num_controllers;
    } else {
        gvt_participants = n_cores;
    }
=======
	if(rootsim_config.num_controllers > 0) {
		gvt_participants = rootsim_config.num_controllers;
	} else {
		gvt_participants = n_cores;
	}

	timer_start(gvt_timer);
}
>>>>>>> origin/power

	timer_start(gvt_timer);

	// Initialize the CCGS subsystem
	ccgs_init();
}

void update_participants(void) {
        gvt_participants = rootsim_config.num_controllers;
}

/**
* Finalizer of the GVT subsystem.
*/
void gvt_fini(void) {

	// Finalize the CCGS subsystem
	ccgs_fini();

<<<<<<< HEAD
    #ifdef HAVE_MPI
	if ((kernel_phase == kphase_idle && !master_thread() && gvt_init_pending()) || kernel_phase == kphase_start) {
=======
#ifdef HAVE_MPI
	if((atomic_read(&kernel_phase) == kphase_idle && !master_thread() && gvt_init_pending())
	   || atomic_read(&kernel_phase) == kphase_start) {
>>>>>>> origin/atomic
		join_white_msg_redux();
		wait_white_msg_redux();
		join_gvt_redux(-1.0);
	}
	else if(atomic_read(&kernel_phase) == kphase_white_msg_redux || atomic_read(&kernel_phase) == kphase_kvt){
		wait_white_msg_redux();
		join_gvt_redux(-1.0);
	}
    #endif
}

<<<<<<< HEAD
=======

inline size_t get_cancelback_threshold() {

	return (size_t)(tot_pages * CANCELBACK_MEM_THRESHOLD);
}


>>>>>>> origin/cancelback
/**
 * This function returns the last computed GVT value at each thread.
 * It can be safely used concurrently to keep track of the evolution of
 * the committed trajectory. It's so far mainly used for termination
 * detection based on passed simulation time.
 */
<<<<<<< HEAD
inline simtime_t get_last_gvt(void) {
<<<<<<< HEAD

    /* TODO: è un po' una porcata, perché potrebbe esserci una corsa
       critica su new_gvt, ma è da verificare */
    if(Threads[tid]->incarnation == THREAD_PROCESSING){
        return new_gvt;
    }
=======
inline simtime_t get_last_gvt(void)
{
<<<<<<< HEAD
	if(last_gvt != new_gvt)
		return new_gvt; // this possibly breaks GVT algorithm in a corner case which I don't remember!
>>>>>>> origin/energy
=======
>>>>>>> origin/energy_tmp
=======
	// TODO: è un po' una porcata, perché potrebbe esserci una corsa
	// critica su new_gvt, ma è da verificare
	if(Threads[tid]->incarnation == THREAD_PROCESSING)
		return new_gvt;
>>>>>>> origin/power
	return last_gvt;
}

<<<<<<< HEAD
static inline void reduce_local_gvt(void) {


<<<<<<< HEAD
	foreach_bound_lp(lp) {
		// If no message has been processed, local estimate for
		// GVT is forced to 0.0. This can happen, e.g., if
		// GVT is computed very early in the run
		if (unlikely(lp->last_processed == NULL)) {
			local_min[local_tid] = 0.0;
            break;
=======
		for(i = 0; i < n_prc_per_thread; i++) {
			if(LPS_bound(i)->bound == NULL) {
				local_min[tid] = 0.0;
				break;
			}
//			local_min[tid] = min(local_min[tid], LPS_bound(i)->bound->timestamp);
			local_min[tid] = min(local_min[tid], LPS_bound(i)->last_sent_time);
>>>>>>> origin/power
		}

		// GVT inheritance: if the current LP has no scheduled
		// events, we can safely assume that it should not
		// participate to the computation of the GVT, because any
		// event to it will appear *after* the GVT
		// FIXME: this condition in an asymmetric execution
		// does not make much sense, because we have a double
		// speculative execution part
		//if (lp->last_processed->next == NULL)
		//	continue;

<<<<<<< HEAD
        local_min[local_tid] = min(local_min[local_tid], lp->last_processed->timestamp);
        local_min[local_tid] = min(local_min[local_tid], lp->bound->timestamp);

     //   printf("ROUND: %u, Thread:%d, gid:%u, last_proc:%f, bound_ts:%f\n", current_GVT_round,local_tid,lp->gid.to_int,lp->last_processed->timestamp,lp->bound->timestamp);

    }
=======
		local_min[local_tid] =
		    min(local_min[local_tid], lp->bound->timestamp);
	}
	atomic_thread_fence(memory_order_acquire);
>>>>>>> origin/atomic
}

<<<<<<< HEAD

simtime_t GVT_phases(void) {
=======
/**
 * This function returns whether Cancelback started during the previous iteration
 */
inline bool has_cancelback_started() {
	return cancelback_flag == 1;
}
>>>>>>> origin/cancelback

	unsigned int i;

	if (thread_phase == tphase_A) {

        #ifdef HAVE_MPI
		// Check whether we have new ingoing messages sent by remote instances
		receive_remote_msgs();
        #endif
		process_bottom_halves();

		reduce_local_gvt();

		thread_phase = tphase_send;	// Entering phase send
		atomic_dec(&counter_A);	// Notify finalization of phase A

        return -1.0;
=======
	if(thread_phase == tphase_send && atomic_read(&counter_A) == 0) {
		// In the asymmetric version, repeating a main loop is such that
		// the output port is completely emptied in the next iteration of the main
		// loop
		if(rootsim_config.num_controllers == 0) {
			#ifdef HAVE_MPI
			// Check whether we have new ingoing messages sent by remote instances
			receive_remote_msgs();
			#endif
			process_bottom_halves();
			schedule();
		} else {
			asym_extract_generated_msgs();
			process_bottom_halves();
		}
		thread_phase = tphase_B;
		atomic_dec(&counter_send);
		return  -1.0;
>>>>>>> origin/power
	}

	if (thread_phase == tphase_send && atomic_read(&counter_A) == 0) {
        /**
        * In the asymmetric version, repeating a main loop is such that
        * the output port is completely emptied in the next iteration of the main
        * loop
        */
        if(rootsim_config.num_controllers == 0) {
        #ifdef HAVE_MPI
            // Check whether we have new ingoing messages sent by remote instances
            receive_remote_msgs();
        #endif
            process_bottom_halves();
            schedule();
        } else {
            asym_extract_generated_msgs();
            process_bottom_halves();
        }

        thread_phase = tphase_B;
        atomic_dec(&counter_send);

        return -1.0;
    }

	if (thread_phase == tphase_B && atomic_read(&counter_send) == 0) {
        #ifdef HAVE_MPI
		// Check whether we have new ingoing messages sent by remote instances
		receive_remote_msgs();
        #endif
		process_bottom_halves();

<<<<<<< HEAD
		reduce_local_gvt();

        #ifdef HAVE_MPI
		// WARNING: local thread cannot send any remote
		// message between the two following calls
		exit_red_phase();
		local_min[local_tid] =
		    min(local_min[local_tid], min_outgoing_red_msg[local_tid]);
        #endif
=======
		for(i = 0; i < n_prc_per_thread; i++) {
			if(LPS_bound(i)->bound == NULL) {
				local_min[tid] = 0.0;
				break;
			}

//			local_min[tid] = min(local_min[tid], LPS_bound(i)->bound->timestamp);
			local_min[tid] = min(local_min[tid], LPS_bound(i)->last_sent_time);
		}

		#ifdef HAVE_MPI
		// WARNING: local thread cannot send any remote
		// message between the two following calls
		exit_red_phase();
		local_min[tid] = min(local_min[tid], min_outgoing_red_msg[tid]);
		#endif
>>>>>>> origin/power

		thread_phase = tphase_aware;
		atomic_dec(&counter_B);


        if (atomic_read(&counter_B) == 0) {
			simtime_t agreed_vt = INFTY;
<<<<<<< HEAD
<<<<<<< HEAD
            for (i = 0; i < gvt_participants; i++) {
                agreed_vt = min(local_min[i], agreed_vt);
=======
			for (i = 0; i < active_threads; i++) {
=======
			for(i = 0; i < gvt_participants; i++) {
>>>>>>> origin/power
				agreed_vt = min(local_min[i], agreed_vt);
>>>>>>> origin/energy
			}
			return agreed_vt;
		}
		return -1.0;
	}

	return -1.0;
}


bool start_new_gvt(void) {

    #ifdef HAVE_MPI
	if (!master_kernel()) {
		//Check if we received a new GVT init msg
		return gvt_init_pending();
	}
    #endif

	// Has enough time passed since the last GVT reduction?
	return timer_value_milli(gvt_timer) >
	    (int)rootsim_config.gvt_time_period;
}

/**
* This is the entry point from the main simulation loop to the GVT subsystem.
* This function is not executed in case of a serial simulation, and is executed
* concurrently by different worker threads in case of a parallel one.
* All the operations here implemented must be re-entrant.
* Any state variable of the GVT implementation must be declared statically and globally
* (in case, on a per-thread basis).
* This function is called at every simulation loop, so at the beginning the code should
* check whether a GVT computation is occurring, or if a computation must be started.
*
* @return The newly computed GVT value, or -1.0. Only a Master Thread should return a value
* 	  different from -1.0, to avoid generating too much information. If every thread
* 	  will return a value different from -1.0, nothing will be broken, but all the values
* 	  will be shown associated with the same kernel id (no way to distinguish between
* 	  different threads here).
*/
simtime_t gvt_operations(void) {
<<<<<<< HEAD
=======
	register unsigned int i;
	simtime_t new_gvt;
	simtime_t new_min_barrier;
	state_t *tentative_barrier;
>>>>>>> origin/cancelback

    // GVT reduction initialization.
	// This is different from the paper's pseudocode to reduce
	// slightly the number of clock reads
<<<<<<< HEAD
<<<<<<< HEAD
	if (kernel_phase == kphase_idle) {
=======
	if(GVT_flag == 0 && atomic_read(&counter_end) == 0) {

<<<<<<< HEAD

		// When using ULT, creating stacks might require more time than
		// the first gvt phase. In this case, we enter the GVT reduction
		// before running INIT. This makes all the assumptions about the
		// fact that bound is not null fail, and everything here inevitably
		// crashes. This is a sanity check for this.
		if(first_gvt_invocation) {
			first_gvt_invocation = false;
			timer_restart(gvt_timer);
		}
>>>>>>> origin/cancelback

		if (start_new_gvt() && iCAS(&current_GVT_round, my_GVT_round, my_GVT_round + 1)) {
=======
	if(atomic_read(&kernel_phase) == kphase_idle) {
>>>>>>> origin/atomic

=======
		if (start_new_gvt() &&
<<<<<<< HEAD
		    iCAS(&current_GVT_round, my_GVT_round, my_GVT_round + 1)) {
						
>>>>>>> origin/energy_tmp
=======
			cmpxchg(&current_GVT_round, &my_GVT_round, my_GVT_round + 1)) {

>>>>>>> origin/atomic
			timer_start(gvt_round_timer);

		    #ifdef HAVE_MPI
			//inform all the other kernels about the new gvt
			if (master_kernel()) {
				broadcast_gvt_init(current_GVT_round);
			} else {
				gvt_init_clear();
			}
            #endif

			// Reduce the current CCGS termination detection
			ccgs_reduce_termination();

			/* kernel GVT round setup */

            #ifdef HAVE_MPI
			flush_white_msg_recv();

<<<<<<< HEAD
			init_kvt_tkn = 1;
			commit_gvt_tkn = 1;
            #endif
=======
			atomic_set(&init_kvt_tkn, 1);
			atomic_set(&commit_gvt_tkn,  1);
			#endif
>>>>>>> origin/atomic

			atomic_set(&init_completed_tkn,  1);
			atomic_set(&commit_kvt_tkn, 1);
			atomic_set(&idle_tkn, 1);

<<<<<<< HEAD
            atomic_set(&counter_initialized, gvt_participants);
			atomic_set(&counter_kvt, gvt_participants);
			atomic_set(&counter_finalized, gvt_participants);

<<<<<<< HEAD
			atomic_set(&counter_A, gvt_participants);
			atomic_set(&counter_send, gvt_participants);
			atomic_set(&counter_B, gvt_participants);
=======
			atomic_set(&counter_initialized, active_threads);
			atomic_set(&counter_kvt, active_threads);
			atomic_set(&counter_finalized, active_threads);

			atomic_set(&counter_A, active_threads);
			atomic_set(&counter_send, active_threads);
			atomic_set(&counter_B, active_threads);
>>>>>>> origin/energy
=======
			atomic_set(&counter_initialized, gvt_participants);
			atomic_set(&counter_kvt, gvt_participants);
			atomic_set(&counter_finalized, gvt_participants);

			atomic_set(&counter_A, gvt_participants);
			atomic_set(&counter_send, gvt_participants);
			atomic_set(&counter_B, gvt_participants);
>>>>>>> origin/power

			atomic_set(&kernel_phase, kphase_start);

<<<<<<< HEAD
			timer_restart(gvt_timer);
		}
	}

	/* Thread setup phase:
	 * each thread needs to setup its own local context
	 * before to partecipate to the new GVT round */
	if( atomic_read(&kernel_phase) == kphase_start && thread_phase == tphase_idle ){

		// Someone has modified the GVT round (possibly me).
		// Keep track of this update
		my_GVT_round = current_GVT_round;

		#ifdef HAVE_MPI
		enter_red_phase();
        #endif

		local_min[tid] = INFTY;

		thread_phase = tphase_A;
		atomic_dec(&counter_initialized);
<<<<<<< HEAD
		if (atomic_read(&counter_initialized) == 0) {
			if (iCAS(&init_completed_tkn, 1, 0)) {
        #ifdef HAVE_MPI
				join_white_msg_redux();
				kernel_phase = kphase_white_msg_redux;
        #else
				kernel_phase = kphase_kvt;
        #endif
=======
			for(i = 0; i < n_prc_per_thread; i++) {
				if(LPS_bound[i]->bound == NULL) {
					local_min[tid] = 0.0;
					local_min_barrier[tid] = 0.0;
					break;
				}

				local_min[tid] = min(local_min[tid], LPS_bound[i]->bound->timestamp);
				tentative_barrier = find_time_barrier(LPS_bound[i]->lid, LPS_bound[i]->bound->timestamp);
				local_min_barrier[tid] = min(local_min_barrier[tid], tentative_barrier->lvt);
>>>>>>> origin/cancelback
=======
		if(atomic_read(&counter_initialized) == 0){
			if(cmpxchg(&init_completed_tkn, &one, 0)){
				#ifdef HAVE_MPI
				join_white_msg_redux();
				atomic_set(&kernel_phase, kphase_white_msg_redux);
				#else
				atomic_set(&kernel_phase, kphase_kvt);
				#endif
>>>>>>> origin/atomic
			}
		}

		return -1.0;
	}

<<<<<<< HEAD

    #ifdef HAVE_MPI
	if (kernel_phase == kphase_white_msg_redux
	    && white_msg_redux_completed() && all_white_msg_received()) {
		if (iCAS(&init_kvt_tkn, 1, 0)) {
=======
#ifdef HAVE_MPI
	if( atomic_read(&kernel_phase) == kphase_white_msg_redux && white_msg_redux_completed() && all_white_msg_received() ){
		if(cmpxchg(&init_kvt_tkn, &one, 0)){
>>>>>>> origin/atomic
			flush_white_msg_sent();
			atomic_set(&kernel_phase, kphase_kvt);
		}
<<<<<<< HEAD
		return -1.0;
	}
    #endif

	/* KVT phase:
	 * make all the threads agree on a common virtual time for this kernel */
<<<<<<< HEAD
	if (kernel_phase == kphase_kvt && thread_phase != tphase_aware) {
 		simtime_t kvt = GVT_phases();
		if (D_DIFFER(kvt, -1.0)) {
            if (iCAS(&commit_kvt_tkn, 1, 0)) {
    #ifdef HAVE_MPI
=======
	if( atomic_read(&kernel_phase) == kphase_kvt && thread_phase != tphase_aware ) {
		simtime_t kvt = GVT_phases();
		if( D_DIFFER(kvt, -1.0) ){
			if(cmpxchg(&commit_kvt_tkn, &one, 0)) {

#ifdef HAVE_MPI
>>>>>>> origin/atomic
				join_gvt_redux(kvt);
				atomic_set(&kernel_phase, kphase_gvt_redux);

                #else
				new_gvt = kvt;
				atomic_set(&kernel_phase, kphase_fossil);

                #endif
=======

		if(my_phase == phase_B && atomic_read(&counter_send) == 0) {
			process_bottom_halves();

			for(i = 0; i < n_prc_per_thread; i++) {
				if(LPS_bound[i]->bound == NULL) {
					local_min[tid] = 0.0;
					local_min_barrier[tid] = 0.0;
					break;
				}

				local_min[tid] = min(local_min[tid], LPS_bound[i]->bound->timestamp);
				tentative_barrier = find_time_barrier(LPS_bound[i]->lid, LPS_bound[i]->bound->timestamp);
				local_min_barrier[tid] = min(local_min_barrier[tid], tentative_barrier->lvt);
>>>>>>> origin/cancelback
			}
		}
		return -1.0;
	}

<<<<<<< HEAD
    #ifdef HAVE_MPI
	if (kernel_phase == kphase_gvt_redux && gvt_redux_completed()) {
		if (iCAS(&commit_gvt_tkn, 1, 0)) {
=======
#ifdef HAVE_MPI
	if( atomic_read(&kernel_phase) == kphase_gvt_redux && gvt_redux_completed() ){
		if(cmpxchg(&commit_gvt_tkn, &one, 0)){
>>>>>>> origin/atomic
			int gvt_round_time = timer_value_micro(gvt_round_timer);
			statistics_post_data(current, STAT_GVT_ROUND_TIME, gvt_round_time);

<<<<<<< HEAD
			new_gvt = last_reduced_gvt();
			atomic_set(&kernel_phase, kphase_fossil);
		}
		return -1.0;
	}
    #endif

	/* GVT adoption phase:
	 * the last agreed GVT needs to be adopted by every thread */
	if( atomic_read(&kernel_phase) == kphase_fossil && thread_phase == tphase_aware ){

  		// Execute fossil collection and termination detection
		// Each thread stores the last computed value in last_gvt,
		// while the return value is the gvt only for the master
		// thread. To check for termination based on simulation time,
		// this variable must be explicitly inspected using
		// get_last_gvt()
		adopt_new_gvt(new_gvt);

		// Dump statistics
		statistics_on_gvt(new_gvt);

		last_gvt = new_gvt;

		thread_phase = tphase_idle;
		atomic_dec(&counter_finalized);

<<<<<<< HEAD
<<<<<<< HEAD
        if (atomic_read(&counter_finalized) == 0) {
			if (iCAS(&idle_tkn, 1, 0)) {
=======
		if(atomic_read(&counter_finalized) == 0){
			if(iCAS(&idle_tkn, 1, 0)){
				// Notify the power cap module that a new statistic sample is available 
				if(rootsim_config.num_controllers > 0)
					gvt_interval_passed = 1; 
				
>>>>>>> origin/power
				kernel_phase = kphase_idle;
<<<<<<< HEAD
                updated_gvt = last_gvt;
                // Notify the power cap module that a new statistic sample is available
            /*    if(rootsim_config.num_controllers > 0)
                    gvt_interval_passed = 1         */      //COINVOLGE power.h
			}
=======
		if(my_phase == phase_aware && atomic_read(&counter_B) == 0) {
			new_gvt = INFTY;
			new_min_barrier = INFTY;

			for(i = 0; i < n_cores; i++) {
				new_gvt = min(local_min[i], new_gvt);
				new_min_barrier = min(local_min_barrier[i], new_min_barrier);
			}

			atomic_dec(&counter_aware);

			if(atomic_read(&counter_aware) == 0) {
				// The last one passing here, resets GVT flag
				iCAS(&GVT_flag, 1, 0);
				// Also, for convenience, the last one will start the Cancelback protocol, if needed
				iCAS(&cancelback_tid, UINT_MAX, tid);
			}

			// Execute fossil collection and termination detection
			// Each thread stores the last computed value in last_gvt,
			// while the return value is the gvt only for the master
			// thread. To check for termination based on simulation time,
			// this variable must be explicitly inspected using
			// get_last_gvt()
			adopt_new_gvt(new_gvt, new_min_barrier);
			adopted_last_gvt = new_gvt;

			if (tid == cancelback_tid) {
				if (is_memory_limit_exceeded() && atomic_read(&counter_cancelback) == 0 && attempt_cancelback(adopted_last_gvt)) { // or last_gvt?
					atomic_set(&counter_cancelback, CANCELBACK_COOLDOWN);
					iCAS(&cancelback_flag, 0, 1);
				} else {
					iCAS(&cancelback_flag, 1, 0);
				}

				if (atomic_read(&counter_cancelback) > 0) {
					atomic_dec(&counter_cancelback);
					// Reset tid to invalid value
					iCAS(&cancelback_tid, tid, UINT_MAX);
				}
=======
				trigger_rebinding();
>>>>>>> origin/energy
=======
		if(atomic_read(&counter_finalized) == 0){
			if(cmpxchg(&idle_tkn, &one, 0)){
				atomic_set(&kernel_phase, kphase_idle);
>>>>>>> origin/atomic
			}

			// Dump statistics
			statistics_post_other_data(STAT_GVT, new_gvt);

			my_phase = phase_end;

			return last_gvt;
		}


	} else {

		// GVT flag is not set. We check whether we can reset the
		// internal thread's state, waiting for the beginning of a
		// new phase.
		if(my_phase == phase_end) {

			// Back to phase A for next GVT round
			my_phase = phase_A;
			local_min[tid] = INFTY;
			local_min_barrier[tid] = INFTY;
			atomic_dec(&counter_end);
			last_gvt = adopted_last_gvt;
>>>>>>> origin/cancelback
		}
		return last_gvt;
	}
	return -1.0;
}
<<<<<<< HEAD
<<<<<<< HEAD

bool is_idle(void){
    if (kernel_phase == kphase_idle){
        return true;
    }
    return false;
}

void update_GVT(void){
    my_GVT_round = current_GVT_round;
    last_gvt = updated_gvt;
}
=======
>>>>>>> origin/cancelback
=======

>>>>>>> origin/atomic
