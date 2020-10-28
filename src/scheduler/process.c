/**
 * @file scheduler/process.c
 *
 * @brief Generic LP management functions
 *
 * Generic LP management functions
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
 *
 * @date December 14, 2017
 */

#include <stdlib.h>
#include <limits.h>

#include <core/core.h>
#include <core/init.h>
#include <scheduler/process.h>
#include <scheduler/scheduler.h>
#include <mm/mm.h>

// TODO: see issue #121 to see how to make this ugly hack disappear
__thread unsigned int __lp_counter = 0;
__thread unsigned int __lp_bound_counter = 0;

/// Maintain LPs' simulation and execution states
struct lp_struct **lps_blocks = NULL;

/** Each KLT(CT) has a binding towards some LPs. This is the structure used
 *  to keep track of LPs currently being handled
 */
<<<<<<< HEAD
__thread struct lp_struct **lps_bound_blocks = NULL;
=======
__thread LP_State **lps_bound_blocks = NULL;

__thread LP_State **asym_lps_mask = NULL;

void initialize_control_blocks(void) {
	register unsigned int i;
>>>>>>> origin/power

__thread struct lp_struct **asym_lps_mask = NULL;


void initialize_binding_blocks(void) {
	lps_bound_blocks = (struct lp_struct **)rsalloc(n_prc * sizeof(struct lp_struct *));
	bzero(lps_bound_blocks, sizeof(struct lp_struct *) * n_prc);

    asym_lps_mask = (struct lp_struct **)rsalloc(n_prc * sizeof(struct lp_struct *));
    bzero(asym_lps_mask, sizeof(struct lp_struct *) * n_prc);

}

<<<<<<< HEAD
void free_binding_blocks(void){
    rsfree(lps_bound_blocks);
    rsfree(asym_lps_mask);
=======
void initialize_binding_blocks(void) {
	lps_bound_blocks = (LP_State **)rsalloc(n_prc * sizeof(LP_State *));
	bzero(lps_bound_blocks, sizeof(LP_State *) * n_prc);

	// Also initialize the mask for asym_schedule used in asymmetric executions
	asym_lps_mask = (LP_State **)rsalloc(n_prc * sizeof(LP_State *));
	bzero(asym_lps_mask, sizeof(LP_State *) * n_prc);
>>>>>>> origin/power
}


<<<<<<< HEAD
void initialize_lps(void) {
	unsigned int i, j;
	unsigned int lid = 0;
	struct lp_struct *lp;
	unsigned int local = 0;
	GID_t gid;

	// First of all, determine what LPs should be locally hosted.
	// Only for them, we are creating struct lp_structs here.
	distribute_lps_on_kernels();

	// We now know how many LPs should be locally hosted. Prepare
	// the place for their control blocks.
	lps_blocks =
	    (struct lp_struct **)rsalloc(n_prc * sizeof(struct lp_struct *));

	// We now iterate over all LP Gids. Everytime that we find an LP
	// which should be locally hosted, we create the local lp_struct
	// process control block.
	for (i = 0; i < n_LP_tot; i++) {
		set_gid(gid, i);
		if (find_kernel_by_gid(gid) != kid)
			continue;

		// Initialize the control block for the current lp
		lp = (struct lp_struct *)rsalloc(sizeof(struct lp_struct));
		bzero(lp, sizeof(struct lp_struct));
		lps_blocks[local++] = lp;

		if (local > n_prc) {
			printf("reached local %d\n", local);
			fflush(stdout);
			abort();
		}
		// We sequentially assign lids, and use the current gid
		lp->lid.to_int = lid++;
		lp->gid = gid;

		// Initialize memory map
		allocator_init(lp);

		lp->slab = slab_init(SLAB_MSG_SIZE);
=======
inline int LPS_bound_foreach(int (*f)(LID_t, GID_t, unsigned int, void *), void *data) {
		LID_t lid;
		GID_t gid;
		unsigned int i;
	
		int ret = 0;

		for(i = 0; i < n_prc_per_thread; i++) {
		lid = LPS_bound(i)->lid;
				gid = LidToGid(lid);
				ret = f(lid, gid, lid_to_int(lid), data);
				if(ret != 0)
						break;
		}

		return ret;
>>>>>>> origin/power

		// Allocate memory for the outgoing buffer
		lp->outgoing_buffer.max_size = INIT_OUTGOING_MSG;
		lp->outgoing_buffer.outgoing_msgs =
		    rsalloc(sizeof(msg_t *) * INIT_OUTGOING_MSG);

<<<<<<< HEAD
		// Initialize bottom halves msg channel
		lp->bottom_halves = init_channel();

		// Which version of OnGVT and ProcessEvent should we use?
<<<<<<< HEAD
		if (rootsim_config.snapshot == SNAPSHOT_FULL || rootsim_config.snapshot == SNAPSHOT_HARDINC) {
			lp->OnGVT = &OnGVT;
			lp->ProcessEvent = &ProcessEvent;
		} else if(rootsim_config.snapshot == SNAPSHOT_SOFTINC) {
			lp->OnGVT = &OnGVT_instr;
			lp->ProcessEvent = &ProcessEvent_instr;
		} else {
			rootsim_error(true, "Wrong type of snapshot: neither full nor incremental\n");
		}
=======
		if (rootsim_config.snapshot == SNAPSHOT_FULL) {
			lp->ProcessEvent = &ProcessEvent_light;
		} // TODO: add here an else for ISS
>>>>>>> origin/termination

		// Allocate LP stack
		lp->stack = get_ult_stack(LP_STACK_SIZE);

		// Set the initial checkpointing period for this LP.
		// If the checkpointing period is fixed, this will not change during the
		// execution. Otherwise, new calls to this function will (locally) update
		// this.
		set_checkpoint_period(lp, rootsim_config.ckpt_period);

		// Initially, every LP is ready
		lp->state = LP_STATE_READY;

		// There is no current state layout at the beginning
		lp->current_base_pointer = NULL;

		// Initialize the queues
		lp->queue_in = new_list(msg_t);
		lp->queue_out = new_list(msg_hdr_t);
		lp->queue_states = new_list(state_t);
        lp->retirement_queue = new_list(msg_t);
		lp->rendezvous_queue = new_list(msg_t);

		// No event has been processed so far
		lp->bound = NULL;

		// We have no information about messages still to be delivered to this LP
		lp->outgoing_buffer.min_in_transit = rsalloc(sizeof(simtime_t) * n_cores);
		for (j = 0; j < n_cores; j++) {
			lp->outgoing_buffer.min_in_transit[j] = INFTY;
		}

#ifdef HAVE_CROSS_STATE
		// No read/write dependencies open so far for the LP. The current lp is always opened
		lp->ECS_index = 0;
		lp->ECS_synch_table[0] = LidToGid(lp);	// LidToGid for distributed ECS
#endif

		// Create User-Level Thread
		context_create(&lp->context, LP_main_loop, NULL, lp->stack, LP_STACK_SIZE);
=======
// In asymmetric executions, it cycles on all LPs currently bound to the controller
// thread. It skips NULL pointers as they represent LP's mapped to ports which are already
// filled. 
inline int LPS_asym_mask_foreach(int (*f)(LID_t, GID_t, unsigned int, void *), void *data) {
		LID_t lid;
		GID_t gid;
		unsigned int i;
		LP_State *mask;
	
		int ret = 0;

		for(i = 0; i < n_prc_per_thread; i++) {
			if((mask = LPS_bound_mask(i)) != NULL){
				lid = mask->lid;
				gid = LidToGid(lid);
				ret = f(lid, gid, lid_to_int(lid), data);
				if(ret != 0)
					break;
			}
		}

		return ret;
}

inline int LPS_foreach(int (*f)(LID_t, GID_t, unsigned int, void *), void *data) {
	LID_t lid;
	GID_t gid;
	unsigned int i;
	int ret = 0;

	for(i = 0; i < n_prc; i++) {
		set_lid(lid, i);
		gid = LidToGid(lid);
		ret = f(lid, gid, i, data);
		if(ret != 0) 
			break;
>>>>>>> origin/power
	}
}

// This works only for locally-hosted LPs!
struct lp_struct *find_lp_by_gid(GID_t gid) {
	foreach_lp(lp) {
		if (lp->gid.to_int == gid.to_int)
			return lp;
	}
	return NULL;
}

void update_last_processed(void){
    foreach_bound_lp(lp){
        if (lp->last_processed != lp->next_last_processed) {
            lp->last_processed = lp->next_last_processed;
        }
    }
}