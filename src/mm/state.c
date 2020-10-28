/**
* @file mm/state.c
*
* @brief LP state management
*
* The state module is responsible for managing LPs' simulation states.
* In particular, it allows to take a snapshot, to restore a previous snapshot,
* and to silently re-execute a portion of simulation events to bring
* a LP to a partiuclar LVT value for which no simulation state is available
* in the log chain.
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
*/

#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <arch/thread.h>
#include <core/core.h>
#include <core/init.h>
#include <core/timer.h>
#include <gvt/ccgs.h>
#include <datatypes/list.h>
#include <scheduler/process.h>
#include <scheduler/scheduler.h>
#include <mm/dymelor.h>
#include <mm/state.h>
#include <mm/globvars.h>
#include <communication/communication.h>
<<<<<<< HEAD
#include <mm/mm.h>
<<<<<<< HEAD
=======
>>>>>>> origin/globvars
=======
#include <mm/dymelor.h>
>>>>>>> origin/incremental
#include <statistics/statistics.h>

<<<<<<< HEAD
=======

#define FORCE_FCF_COUNTER 20

/// Function pointer to switch between the parallel and serial version of SetState
void (*SetState)(void *new_state);

#define AUDIT if(0)
#define AUDIT1 if(0)

>>>>>>> origin/power
/**
* This function is used to create a state log to be added to the LP's log chain
*
* @param lp A pointer to the lp_struct of the LP for which a checkpoint
*           is to be taken.
*/
<<<<<<< HEAD
bool LogState(struct lp_struct *lp)
{
=======
bool LogState(LID_t lid) {
>>>>>>> origin/power
	bool take_snapshot = false;
<<<<<<< HEAD
	state_t *new_state;
=======

	state_t new_state; // If inserted, list API makes a copy of this
>>>>>>> origin/reverse

	if (unlikely(is_blocked_state(lp->state))) {
		return take_snapshot;
	}
	// Keep track of the invocations to LogState
	lp->from_last_ckpt++;

	if (lp->state_log_forced) {
		lp->state_log_forced = false;
		lp->from_last_ckpt = 0;

		take_snapshot = true;
		goto skip_switch;
	}
	// Switch on the checkpointing mode
	switch (rootsim_config.checkpointing) {

		case STATE_SAVING_COPY:
			take_snapshot = true;
			break;

		case STATE_SAVING_PERIODIC:
			if (lp->from_last_ckpt >= lp->ckpt_period) {
				take_snapshot = true;
				lp->from_last_ckpt = 0;
			}
			break;

		default:
			rootsim_error(true, "State saving mode not supported.");
	}

 skip_switch:

	// Shall we take a log?
	if (take_snapshot) {

		// Check if we have to force a full checkpoint
		lp->from_last_full_ckpt++;
		if(lp->from_last_full_ckpt >= 10) {
			set_force_full(lp);
			lp->from_last_full_ckpt = 0;
		}

		// Allocate the state buffer
		new_state = rsalloc(sizeof(*new_state));

		// Associate the checkpoint with current LVT and last-executed event
		if(rootsim_config.num_controllers>0) {
            new_state->lvt = lp->next_last_processed->timestamp;
            new_state->last_event = lp->next_last_processed;
        }
		else{
            new_state->lvt = lp->last_processed->timestamp;
            new_state->last_event = lp->last_processed;
		}

		// Log simulation model buffers
		new_state->checkpoint_i = allocator_checkpoint_take(lp);

		// Log members of lp_struct which must be restored
		new_state->state = lp->state;
		new_state->base_pointer = lp->current_base_pointer;

		// Early evaluation of simulation termination.
		new_state->simulation_completed = ccgs_lp_can_halt(lp);

<<<<<<< HEAD
		// Log library-related states
		memcpy(&new_state->numerical, &lp->numerical, sizeof(numerical_state_t));

<<<<<<< HEAD
		if(&topology_settings && topology_settings.write_enabled) {
			new_state->topology = rsalloc(topology_global.chkp_size);
			memcpy(new_state->topology, lp->topology, topology_global.chkp_size);
		}

		if(&abm_settings) {
			new_state->region_data = abm_do_checkpoint(lp->region);
		}

=======
>>>>>>> origin/approximated
		// Link the new checkpoint to the state chain
		list_insert_tail(lp->queue_states, new_state);
		on_log_state(new_state);
=======
		// list_insert() makes a copy of the payload. We store the pointer to the state in the event as well
		//LPS[lid]->bound->checkpoint_of_event = (struct _state_t *)list_insert_tail(lid, LPS[lid]->queue_states, &new_state);
		LPS[lid]->bound->checkpoint_of_event = list_insert_tail(lid, LPS[lid]->queue_states, &new_state);
		//LPS[lid]->bound->checkpoint_of_event = (void*)list_insert_tail(lid, LPS[lid]->queue_states, &new_state);
		//list_insert_tail(lid, LPS[lid]->queue_states, &new_state);
		//LPS[lid]->bound->checkpoint_of_event = list_tail(LPS[lid]->queue_states);

		//this is for mixed model
	 	LPS[lid]->last_correct_log_time = LPS[lid]->bound->timestamp;
>>>>>>> origin/reverse

	}

	return take_snapshot;
}

<<<<<<< HEAD
void RestoreState(struct lp_struct *lp, state_t * state_to_restore)
=======
void RestoreState(struct lp_struct *lp, state_t *restore_state)
>>>>>>> origin/incremental
{
	//~ printf("(%d) Restoring state at %f\n", lp->gid.to_int, restore_state->lvt);

	// Restore simulation model buffers
<<<<<<< HEAD
	log_restore(lp, state_to_restore);
=======
	allocator_checkpoint_restore(lp, restore_state->checkpoint_i);
>>>>>>> origin/exercise

	// Restore members of lp_struct which have been checkpointed
	lp->current_base_pointer = state_to_restore->base_pointer;
	lp->state = state_to_restore->state;

	// Restore library-related states
	memcpy(&lp->numerical, &state_to_restore->numerical,
	       sizeof(numerical_state_t));

<<<<<<< HEAD
=======
	if(&topology_settings && topology_settings.write_enabled){
		memcpy(lp->topology, state_to_restore->topology,
               topology_global.chkp_size);
	}

	if(&abm_settings)
		abm_restore_checkpoint(state_to_restore->region_data, lp->region);

>>>>>>> origin/asym
#ifdef HAVE_CROSS_STATE
	lp->ECS_index = 0;
	lp->wait_on_rendezvous = 0;
	lp->wait_on_object = 0;
#endif
}

/**
* This function brings the state pointed by "state" to "final time" by re-executing all the events without sending any messages
*
* @author Francesco Quaglia
* @author Alessandro Pellegrini
*
* @param lp A pointer to the LP's lp_struct for which we want to silently
*           reprocess already-executed events
* @param evt A pointer to the event from which start the re-execution
* @param final_evt A pointer to the first event which should *not* be reprocessed in silent execution
*
* @return The number of events re-processed during the silent execution
*/
unsigned int silent_execution(struct lp_struct *lp, msg_t *evt, msg_t *final_evt)
{
	unsigned int events = 0;
	unsigned short int old_state;

	// current state can be either idle READY, BLOCKED or ROLLBACK, so we save it and then put it back in place
	old_state = lp->state;
	lp->state = LP_STATE_SILENT_EXEC;

	// Reprocess events. Outgoing messages are explicitly discarded, as this part of
	// the simulation has been already executed at least once
	if(!evt){
				rootsim_error(true, "DAFAKKK");

			}
	while(evt != final_evt){
		evt = list_next(evt);
		if(!evt){
					rootsim_error(true, "DAFAKKK");
					break;
				}
		if (unlikely(!reprocess_control_msg(evt))) {
			continue;
		}
<<<<<<< HEAD
=======

		events++;
		on_process_event_silent(evt);
>>>>>>> origin/energy_tmp
		activate_LP(lp, evt);
		++events;
	}

	lp->state = old_state;
	return events;
}

/**
* This function rolls back the execution of a certain LP. The point where the
* execution is rolled back is identified by the event pointed by the bound field
* in the LP control block.
* For a rollback operation to take place, that pointer must be set before calling
* this function.
* Upon the execution of a rollback, different algorithms are executed depending on
* whether the reverse computing is active or not.
*
* @author Francesco Quaglia
* @author Alessandro Pellegrini
*
* @param lp A pointer to the lp_struct of the LP to rollback
*/
void rollback(struct lp_struct *lp)
{
	state_t *state_to_restore, *s;
	msg_t *last_correct_event;
	msg_t *last_restored_event;
	unsigned int reprocessed_events;
	msg_t *temp;
	int bound_counter = 0;

	#ifdef HAVE_REVERSE
	state_t *s1;
	msg_t *event_with_log;
	revwin_t *current_revwin, *delete_revwin, *end_revwin;
	#endif

	// Sanity check
<<<<<<< HEAD
	if (unlikely(lp->state != LP_STATE_ROLLBACK)) {
		rootsim_error(false, "I'm asked to roll back LP %d's execution, but rollback_bound is not set. Ignoring...\n",
			      lp->gid.to_int);
=======
	if(LPS(lid)->state != LP_STATE_ROLLBACK) {
		rootsim_error(false, "I'm asked to roll back LP %d's execution, but it's not flagged as an LP to rollback\n", LidToGid(lid));
>>>>>>> origin/power
		return;
	}
<<<<<<< HEAD

	//~ printf("(%d) Rolling back at %f\n", lp->gid.to_int, lvt(lp));

	// Discard any possible execution state related to a blocked execution
	memcpy(&lp->context, &lp->default_context, sizeof(LP_context_t));

	statistics_post_data(lp, STAT_ROLLBACK, 1.0);

	last_correct_event = lp->bound;

<<<<<<< HEAD
=======

	statistics_post_lp_data(lid, STAT_ROLLBACK, 1.0);

	last_correct_event = LPS[lid]->bound;
<<<<<<< HEAD
>>>>>>> origin/cancelback

<<<<<<< HEAD
=======

	globvars_rollback(last_correct_event->timestamp);
	
>>>>>>> origin/globvars
=======
>>>>>>> origin/incremental
=======

>>>>>>> origin/reverse
	// Send antimessages
	send_antimessages(lp, last_correct_event->timestamp);

<<<<<<< HEAD
	on_log_restore();
=======
	// Control messages must be rolled back as well
	rollback_control_message(lid, last_correct_event->timestamp);


	// Reset the ProcessEvent pointer for this event so that when we finish the
	// rollback operation, we have restored the state to an initial condition
	// TODO: we have as wll 
	_ProcessEvent[lid] = ProcessEvent;

	AUDIT printf("rollback of %d at time %f\n",lid,LPS[lid]->bound->timestamp);

	//if (LPS[lid]->current_revwin == NULL) goto FCF_path;

	//goto reverse; //this is for heuristic mixed
	goto FCF_path;
	//goto FULL_reverse;
	//here nothing specified leads to MIXED_path based on model 
	

MIXED_path:


	if(last_correct_event->timestamp <= LPS[lid]->last_correct_log_time){
		goto FCF_path;
	}

	if(last_correct_event->revwin == NULL || LPS[lid]->current_revwin == NULL){
		goto FCF_path;
	}
	
	if(last_correct_event->timestamp > LPS[lid]->last_correct_log_time){
		goto for_mixed_restore;
	}

	goto out;

FULL_reverse:
	//prune the state queue
	LPS[lid]->FCF = 0; 
>>>>>>> origin/reverse
	// Find the state to be restored, and prune the wrongly computed states
<<<<<<< HEAD
	state_to_restore = list_tail(lp->queue_states);
	while (state_to_restore != NULL && state_to_restore->lvt > last_correct_event->timestamp) {	// It's > rather than >= because we have already taken into account simultaneous events
		s = state_to_restore;
        state_to_restore = list_prev(state_to_restore);
=======
	restore_state = list_tail(lp->queue_states);
	while (restore_state != NULL && restore_state->lvt > last_correct_event->timestamp) {	// It's > rather than >= because we have already taken into account simultaneous events
		s = restore_state;
		on_log_discarded(s);
		restore_state = list_prev(restore_state);
<<<<<<< HEAD
>>>>>>> origin/energy_tmp
		log_delete(s->log);
<<<<<<< HEAD
		statistics_post_data(lp, STAT_ABORT, (double)lp->ckpt_period); 
=======
>>>>>>> origin/exercise
#ifndef NDEBUG
		s->last_event = (void *)0xBABEBEEF;
#endif
		list_delete_by_content(lp->queue_states, s);
=======
		if(s->last_event != NULL)  // Per considerare gli eventi eliminati da antimessaggi
			s->last_event->checkpoint_of_event = NULL; // Cannot use anti-dangling here, because we explicitly check for NULL 
		s->last_event = (void *)0xDEADC0DE;
		list_delete_by_content(lid, LPS[lid]->queue_states, s);
	}
	
	//this is for mixed model
	if(restore_state == NULL) {
		printf("panic on log queue\n");
		exit(-1);
	}
	else{
		LPS[lid]->last_correct_log_time = restore_state->lvt;
	}

for_mixed_restore:

	// Navigate from the event list to the revwin list
	current_revwin = LPS[lid]->current_revwin;
	end_revwin = last_correct_event->revwin;
	
	// At this point: the state queue is pruned, and the correct state is restored.	
	// We can thus start to undo events until we reach the end_revwin (excluded)
	while(current_revwin != end_revwin) {
		AUDIT1 printf("before revwin: lid %d - executed events = %d - channel counter %d - arriving calls %d - complete calls %d\n",lid, *(int*)(LPS[lid]->current_base_pointer),*(1+(int*)(LPS[lid]->current_base_pointer)),*(2+(int*)(LPS[lid]->current_base_pointer)),*(3+(int*)(LPS[lid]->current_base_pointer)));
		AUDIT printf("About to execute revwin at %p for lid %d\n", current_revwin,lid);

		execute_undo_event(lid,current_revwin);

		AUDIT printf("Executed revwin at %p for lid %d\n", current_revwin,lid);
		AUDIT1 printf("after revwin: lid %d - executed events = %d - channel counter %d - arriving calls %d - complete calls %d\n",lid, *(int*)(LPS[lid]->current_base_pointer),*(1+(int*)(LPS[lid]->current_base_pointer)),*(2+(int*)(LPS[lid]->current_base_pointer)),*(3+(int*)(LPS[lid]->current_base_pointer)));

		delete_revwin = current_revwin;
		current_revwin = delete_revwin->prev;
		//current_revwin = list_prev(current_revwin);
		//revwin_free(lid,delete_revwin);
		revwin_reset(lid,delete_revwin);
		//revwin_reset(delete_revwin);
	}

	LPS[lid]->current_revwin = NULL;

	goto out;


	//goto FCF_path;

	//if( LPS[lid]->FCF == 1) goto FCF_path;

	//#ifdef HAVE_REVERSE
	// Switch between coasting forward and reverse scrubbing
	//if(!rootsim_config.disable_reverse && last_correct_event->revwin != NULL)
	//	goto reverse;
		goto reverse;
	//#endif

FCF_path:
	LPS[lid]->FCF = 0; 
	// Find the state to be restored, and prune the wrongly computed states
	restore_state = list_tail(LPS[lid]->queue_states);
	while (restore_state != NULL && restore_state->lvt > last_correct_event->timestamp) { // It's > rather than >= because we have already taken into account simultaneous events
		s = restore_state;
		restore_state = list_prev(restore_state);
		log_delete(s->log);
	//	if(s->last_event != NULL)  // Per considerare gli eventi eliminati da antimessaggi
	//		s->last_event->checkpoint_of_event = NULL; // Cannot use anti-dangling here, because we explicitly check for NULL 
		s->last_event = (void *)0xDEADC0DE;
		list_delete_by_content(lid, LPS[lid]->queue_states, s);
>>>>>>> origin/reverse
	}

	// Restore the simulation state and correct the state base pointer
	RestoreState(lp, state_to_restore);

<<<<<<< HEAD
    last_restored_event = state_to_restore->last_event;
	reprocessed_events = silent_execution(lp, last_restored_event, last_correct_event);
	statistics_post_data(lp, STAT_SILENT, (double)reprocessed_events);
statistics_post_data(lp, STAT_ABORT, (double)lp->ckpt_period);
//	if(lp->ckpt_period < reprocessed_events) printf("AHAH %u %u\n", lp->ckpt_period, reprocessed_events);
 	statistics_post_data(lp, STAT_ABORT, (-1.0*reprocessed_events));

	// TODO: silent execution resets the LP state to the previous
	// value, so it should be the last function to be called within rollback()
	// Control messages must be rolled back as well
	rollback_control_message(lp, last_correct_event->timestamp);
=======
	//this is for mixed model
	if(restore_state == NULL) {
		printf("panic on log queue\n");
		exit(-1);
	}
	else{
		LPS[lid]->last_correct_log_time = restore_state->lvt;
	}

	last_restored_event = restore_state->last_event;
	reprocessed_events = silent_execution(lid, LPS[lid]->current_base_pointer, last_restored_event, last_correct_event);
	statistics_post_lp_data(lid, STAT_SILENT, (double)reprocessed_events);

#ifdef HAVE_REVERSE

	goto out;

    reverse:

	event_with_log = last_correct_event;
	bound_counter = 0;

	//while(event_with_log != LPS[lid]->old_bound && event_with_log->checkpoint_of_event == NULL) {
	while(event_with_log && event_with_log->checkpoint_of_event == NULL) {

		//if(event_with_log != NULL)
			AUDIT printf("%p (%f) - ckpt of event is %p\n", event_with_log, event_with_log->timestamp,event_with_log->checkpoint_of_event);

		event_with_log = list_next(event_with_log);
		bound_counter++;
		if (bound_counter > FORCE_FCF_COUNTER){
		//	printf("rollback %d - bound counter reached - sliding to FCF\n",lid); 
		       	goto FCF_path;
		}
	}

	if(event_with_log == NULL){
		printf("falling back to FCF\n");
	       	goto FCF_path;
	}
	else{
	// No state to restore. We're in the last section of the forward execution.
	// We don't need to restore a state, we just undo the latest events
	//if(event_with_log != LPS[lid]->old_bound) {
		// Restore the state and prune the state queue. We delete as well
		// the state that we have restored because from that point we undo
		// events, and thus the state will no longer be correct.
		// There is the possibility that we don't undo any event if the last
		// correct event is the one which is associated with the log which
		// we restored (since we take checkpoints _after_ the execution of
		// an event). Nevertheless, in this case, we just "lose" one correct
		// checkpoint from the checkpoint queue, which is anyhow correct.
		// Explicitly accounting for this case would be costly.
		restore_state = event_with_log->checkpoint_of_event;
		RestoreState(lid, restore_state);

		s = restore_state;
		if(event_with_log == last_correct_event) s = list_next(s);//original for reverse
		//
		//new stuff for model
		/*
		if(event_with_log == last_correct_event){
			       
			LPS[lid]->last_correct_log_time = s->lvt;
			s = list_next(s);//original for reverse
		}
		else{
			LPS[lid]->last_correct_log_time = list_prev(s)->lvt;

		}
		*/
		//new stuff upt to here

		while(s != NULL) {
			log_delete(s->log);
			if(s->last_event != NULL)
					s->last_event->checkpoint_of_event = NULL;
			s->last_event = (void *)0xDEADC0DE;
			s1 = list_next(s);
			list_delete_by_content(lid, LPS[lid]->queue_states, s);
			s = s1;
		}
		/*
		if(event_with_log != last_correct_event) {
			s = restore_state;
			while(s != NULL) {
				log_delete(s->log);
				if(s->last_event != NULL)
					s->last_event->checkpoint_of_event = NULL;
				s->last_event = (void *)0xDEADC0DE;
				s1 = list_next(s);
				list_delete_by_content(lid, LPS[lid]->queue_states, s);
				s = s1;
			}
		}
		*/
	}

	// Navigate from the event list to the revwin list
	current_revwin = event_with_log->revwin;
	end_revwin = last_correct_event->revwin;
	
	// At this point: the state queue is pruned, and the correct state is restored.	
	// We can thus start to undo events until we reach the end_revwin (excluded)
	while(current_revwin != end_revwin) {
		AUDIT1 printf("before revwin: lid %d - executed events = %d - channel counter %d - arriving calls %d - complete calls %d\n",lid, *(int*)(LPS[lid]->current_base_pointer),*(1+(int*)(LPS[lid]->current_base_pointer)),*(2+(int*)(LPS[lid]->current_base_pointer)),*(3+(int*)(LPS[lid]->current_base_pointer)));
		AUDIT printf("About to execute revwin at %p for lid %d\n", current_revwin,lid);

		execute_undo_event(lid,current_revwin);

		AUDIT printf("Executed revwin at %p for lid %d\n", current_revwin,lid);
		AUDIT1 printf("after revwin: lid %d - executed events = %d - channel counter %d - arriving calls %d - complete calls %d\n",lid, *(int*)(LPS[lid]->current_base_pointer),*(1+(int*)(LPS[lid]->current_base_pointer)),*(2+(int*)(LPS[lid]->current_base_pointer)),*(3+(int*)(LPS[lid]->current_base_pointer)));

		delete_revwin = current_revwin;
		current_revwin = delete_revwin->prev;
		//current_revwin = list_prev(current_revwin);
		//revwin_free(lid,delete_revwin);
		revwin_reset(lid, delete_revwin);
	}

	
	
    out:
#endif

	LPS[lid]->old_bound = NULL;
}


>>>>>>> origin/reverse

}

/**
* This function computes a time barrier, namely the first state snapshot
* which is associated with a simulation time <= than the simtime value
* passed as an argument.
* The time barrier, in the runtime environment, is used to safely install
* a new computed GVT.
*
* @author Francesco Quaglia
* @author Alessandro Pellegrini
*
* @param lp A pointer to the lp_struct of the LP for which we are looking
*           for the current time barrier
* @param simtime The simulation time to be associated with a state barrier
* @return A pointer to the state that represents the time barrier
*/
state_t *find_time_barrier(struct lp_struct *lp, simtime_t simtime)
{
	state_t *barrier_state;

	if (unlikely(D_EQUAL(simtime, 0.0))) {
		return list_head(lp->queue_states);
	}

	barrier_state = list_tail(lp->queue_states);

	// Must point to the state with lvt immediately before the GVT
	while (barrier_state != NULL && barrier_state->lvt >= simtime) {
		barrier_state = list_prev(barrier_state);
<<<<<<< HEAD
	}
	if (barrier_state == NULL) {
		barrier_state = list_head(lp->queue_states);
=======
  	}
  	if(barrier_state == NULL) {
		barrier_state = list_head(LPS[lid]->queue_states);
>>>>>>> origin/cancelback
	}

<<<<<<< HEAD

	// Search for the first full log before the gvt
	while(true) {
		if(is_incremental(barrier_state->log) == false)
			break;
		barrier_state = list_prev(barrier_state);
	}

=======
>>>>>>> origin/reverse
	return barrier_state;
}

/**
* This function sets the buffer of the current LP's state
*
* @author Francesco Quaglia
*
* @param new_state The new buffer
*
* @todo malloc wrapper
*/
void SetState(void *new_state)
{
	current->current_base_pointer = new_state;
#ifdef HAVE_APPROXIMATED_ROLLBACK
	if(likely(!rootsim_config.serial))
		CoreMemoryMark(new_state);
#endif
}

/**
* This function sets the checkpoint mode
*
* @author Francesco Quaglia
* @author Alessandro Pellegrini
*
* @param ckpt_mode The new checkpoint mode
*/
void set_checkpoint_mode(int ckpt_mode)
{
	rootsim_config.checkpointing = ckpt_mode;
}

/**
* This function sets the checkpoint period
*
* @author Francesco Quaglia
* @author Alessandro Pellegrini
*
* @param lp A pointer to the LP's lp_struct which should have its
*           checkpoint period changed.
* @param period The new checkpoint period
*/
void set_checkpoint_period(struct lp_struct *lp, int period)
{
	lp->ckpt_period = period;
}

/**
* This function tells the logging subsystem to take a LP state log
* upon the next invocation to LogState(), independently of the current
* checkpointing period
*
* @author Alessandro Pellegrini
*
* @param lp A pointer to the lp_struct of the LP for which the checkpoint
*           shold be forced after the next call to LogState()
*/
void force_LP_checkpoint(struct lp_struct *lp)
{
	lp->state_log_forced = true;
}
