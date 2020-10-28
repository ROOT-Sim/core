/**
* @file queues/queues.c
*
* @brief Message queueing subsystem
*
* This module implements the event/message queues subsystem.
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
* @author Roberto Vitali
* @author Alessandro Pellegrini
*
* @date March 16, 2011
*/

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <scheduler/process.h>
#include <core/core.h>
#include <arch/atomic.h>
#include <arch/thread.h>
#include <datatypes/list.h>
#include <datatypes/msgchannel.h>
#include <queues/queues.h>
#include <mm/state.h>
#include <mm/mm.h>
#include <scheduler/scheduler.h>
#include <communication/communication.h>
#include <communication/gvt.h>
#include <statistics/statistics.h>
#include <gvt/gvt.h>
#include <score/score.h>


///keep the count of straggleres received

__thread double stragglers_received = 0;
__thread bool straggler_set;

/**
* This function return the timestamp of the next-to-execute event
*
* @author Alessandro Pellegrini
* @author Francesco Quaglia
*
* @param lp A pointer to the LP's lp_struct for which we want to discover
*           the timestamp of the next event
* @return The timestamp of the next-to-execute event
*/
simtime_t next_event_timestamp(struct lp_struct *lp)
{
	msg_t *evt;

	/*
#ifdef HAVE_REVERSE
    retry:
#endif
*/
	// The bound can be NULL in the first execution or if it has gone back
<<<<<<< HEAD
	if (unlikely(lp->bound == NULL && !list_empty(lp->queue_in))) {
		return list_head(lp->queue_in)->timestamp;
=======
	if (LPS[id]->bound == NULL && !list_empty(LPS[id]->queue_in)) {
		evt = list_head(LPS[id]->queue_in);
		ret = evt->timestamp;
>>>>>>> origin/reverse
	} else {
		evt = list_next(lp->bound);
		if (likely(evt != NULL)) {
            return evt->timestamp;
		}
	}
<<<<<<< HEAD
    return INFTY;
=======

	/*

#ifdef HAVE_REVERSE
	// we could potentially select for execution an event which was subject to a lazy cancellation.
	// In this case, we delete it here and retry again the extraction of the next event.
	if(evt->marked_by_antimessage) {
		evt->checkpoint_of_event = NULL;
		list_delete_by_content(evt->sender, LPS[id]->queue_in, evt);
		goto retry;
	}
#endif
*/

	return ret;
>>>>>>> origin/reverse

}

/**
* This function advances the pointer to the last correctly executed event (bound).
* It is called right before the execution of it. This means that after this
* call, before the actual call to __ProcessEvent(), bound is pointing to a
* not-yet-executed event. This is the only case where this can happen.
*
* @author Alessandro Pellegrini
* @author Francesco Quaglia
*
* @param lp A pointer to the LP's lp_struct which should have its bound
*           updated in order to point to the next event to be processed
* @return The pointer to the event is going to be processed
*/
msg_t *advance_to_next_event(struct lp_struct *lp)
{
    msg_t *bound = NULL;

    //spin_lock(&lp->bound_lock);
	if (likely(list_next(lp->bound) != NULL)) {
		lp->bound = list_next(lp->bound);
		bound = lp->bound;
	}
    //spin_unlock(&lp->bound_lock);

	return bound;
}

/**
* Insert a message in the bottom half of a locally-hosted LP. Of course,
* the LP must be locally hosted.
*
* @author Alessandro Pellegrini
*
* @param msg The message to be added into some LP's bottom half.
*/
void insert_bottom_half(msg_t * msg)
{
	struct lp_struct *lp = find_lp_by_gid(msg->receiver);

	validate_msg(msg);

    insert_msg(lp->bottom_halves, msg);
#ifdef HAVE_PREEMPTION
	update_min_in_transit(lp->worker_thread, msg->timestamp);
#endif
}

/**
* Process bottom halves received by all the LPs hosted by the current KLT
*
* @author Alessandro Pellegrini
*/
void process_bottom_halves(void)
{
	struct lp_struct *receiver;

	msg_t *msg_to_process;
<<<<<<< HEAD
	msg_t *matched_msg;
    double ts_bound;
=======
	list(msg_t) processing;
>>>>>>> origin/cancelback

    double straggler_percentage;

<<<<<<< HEAD
	foreach_bound_lp(lp) {

	    straggler_set = false;

		while ((msg_to_process = get_msg(lp->bottom_halves)) != NULL) {
			receiver = find_lp_by_gid(msg_to_process->receiver);
=======
		while((msg_to_process = (msg_t *)get_BH(LPS_bound[i]->lid)) != NULL) {
>>>>>>> origin/reverse


<<<<<<< HEAD
            if (unlikely (msg_to_process->timestamp < get_last_gvt())) { // Sanity check
                dump_msg_content(msg_to_process);
                printf("LAST GVT: %f\n", get_last_gvt());
                printf("LP's boundTS: %f\n", receiver->bound->timestamp);
                rootsim_error(true,"I'm receiving a message before the GVT\n");
            }
=======
			switch (msg_to_process->message_kind) {
>>>>>>> origin/reverse

<<<<<<< HEAD
            if (unlikely(!receive_control_msg(msg_to_process))) {   // Handle control messages
				msg_release(msg_to_process);
				continue;
			}
=======
				// It's an antimessage
				case negative:
				case negative_cb:
>>>>>>> origin/cancelback

           /* if(local_tid==1)
                printf("TS:%f, LAST GVT: %f\n", msg_to_process->timestamp, get_last_gvt());*/

<<<<<<< HEAD

            validate_msg(msg_to_process);

			switch (msg_to_process->message_kind) {

			    case negative:  // It's an antimessage
=======
					// Find the message matching the antimessage
                    msg_t *matched_in_msg = NULL;
					msg_t* in_msg = list_tail(LPS[lid_receiver]->queue_in);
					while(in_msg != NULL) {

                        if (in_msg->mark == msg_to_process->mark) {
                            matched_in_msg = in_msg;
                            break;
                        }

						in_msg = list_prev(in_msg);
					}

<<<<<<< HEAD
					if(matched_in_msg == NULL) {
						
						if (has_cancelback_started())
							stylized_printf("Cancelback started!\n", CYAN, true);
						else
							stylized_printf("Cancelback did not start!\n", CYAN, true);

						rootsim_error(false, "LP %d Received an antimessage with mark \033[1;31m%llu\033[0m from LP %u, but no such mark found in the input queue!\n", LPS_bound[i]->lid, msg_to_process->mark, msg_to_process->sender);
=======
					if(matched_msg == NULL) {

						// That's a serious error: we have received an antimessage but
						// we are not able to find a matching positive message. This is
						// an impossible condition, and thus we abort the simulation.
						rootsim_error(false, "LP %d Received an antimessage with mark %llu at LP %u from LP %u, but no such mark found in the input queue!\n", LPS_bound[i]->lid, msg_to_process->mark, msg_to_process->receiver, msg_to_process->sender);
>>>>>>> origin/reverse
						printf("Message Content:"
							"sender: %d\n"
							"receiver: %d\n"
							"type: %d\n"
							"timestamp: %f\n"
							"send time: %f\n"
							"mark: %llu\n"
							"rendezvous mark %llu\n",
							msg_to_process->sender,
							msg_to_process->receiver,
							msg_to_process->type,
							msg_to_process->timestamp,
							msg_to_process->send_time,
							msg_to_process->mark,
							msg_to_process->rendezvous_mark);
						fflush(stdout);
                        abort();

					} else {
						
						// if (msg_to_process->message_kind == negative_cb)
							// printf("LP %u received antimessage \033[1;31m%llu\033[0m from LP %u\n", lid_receiver, msg_to_process->mark, msg_to_process->sender);

                        // If the matched message is in the past, we have to rollback
                        if(matched_in_msg->timestamp <= lvt(lid_receiver)) {

                            LPS[lid_receiver]->bound = list_prev(matched_in_msg);

                            while (LPS[lid_receiver]->bound != NULL && LPS[lid_receiver]->bound->timestamp == msg_to_process->timestamp) {
                                LPS[lid_receiver]->bound = list_prev(LPS[lid_receiver]->bound);
                            }
							
							// if (msg_to_process->message_kind == negative_cb)
							// 	printf("\033[1;35mNegative Cancelback message %llu causing LP %u to rollback\033[0m\n", msg_to_process->mark, LidToGid(lid_receiver));

							if (msg_to_process->message_kind == negative_cb/* && LPS[lid_receiver]->state != LP_STATE_SYNCH_FOR_CANCELBACK*/) {
                            	LPS[lid_receiver]->state = LP_STATE_SYNCH_FOR_CANCELBACK;
								LPS[lid_receiver]->state_to_resume = LP_STATE_ROLLBACK;
							} else if (LPS[lid_receiver]->state != LP_STATE_SYNCH_FOR_CANCELBACK) {
								LPS[lid_receiver]->state = LP_STATE_ROLLBACK;
							}
                        }

                        // Delete the matched message
                        list_delete_by_content(LPS[lid_receiver]->queue_in, matched_in_msg);
                    }

<<<<<<< HEAD
					break;
=======

<<<<<<< HEAD
					#ifdef HAVE_MPI
					register_incoming_msg(msg_to_process);
					#endif

					// If the matched message is in the past, we have to rollback
					if(matched_msg->timestamp <= lvt(lid_receiver)) {
>>>>>>> origin/power

				case positive_cb: ;

                    // Added in order to support deletion of output messages in the Cancelback protocol
                    msg_hdr_t* matched_out_msg = NULL;
                    msg_hdr_t* out_msg = list_tail(LPS[lid_receiver]->queue_out);
                    while(out_msg != NULL) {
=======
						// If the matched message is in the past, we have to rollback
						if(matched_msg->timestamp <= lvt(lid_receiver)) {

#ifdef HAVE_REVERSE
							if(LPS[lid_receiver]->old_bound == NULL)
								LPS[lid_receiver]->old_bound = LPS[lid_receiver]->bound;
#endif

							LPS[lid_receiver]->bound = list_prev(matched_msg);
							LPS[lid_receiver]->state = LP_STATE_ROLLBACK;
	
						} 

						// HERE WE SLIDE ALONG THE ORIGINL PATH
#ifdef HAVE_REVERSE
						// In case we support reverse execution, an event pointing to a log could be a candidate
						// to navigate to the reverse windows list. In this case, therefore, we cannot immediately
						// delete the event from the queue. Rather, we flag it for later deletion.
						// The deletion will happen within the next_event_timestamp() function, when
						// the event is being selected for execution.
						//
						/*
						 
						 if(0 || matched_msg->checkpoint_of_event != NULL) {

							matched_msg->checkpoint_of_event->last_event = NULL;
							matched_msg->marked_by_antimessage = true;
						} else {
							if(matched_msg == LPS[lid_receiver]->old_bound){
								printf("panic\n");
								LPS[lid_receiver]->FCF = 1;

							}	
							*/
							// BUG BUG we need lid_receiver as first param
							//list_delete_by_content(matched_msg->sender, LPS[lid_receiver]->queue_in, matched_msg);
					 	if (matched_msg->checkpoint_of_event != NULL) {
							matched_msg->checkpoint_of_event->last_event = NULL;
						}	
							list_delete_by_content(lid_receiver, LPS[lid_receiver]->queue_in, matched_msg);
					//	}
#else
						// When we're not supporting the reverse execution, we can immediately delete
						// the event matched by an antimessage.
						// BIG BUG we need receiver not sender as first param
						//list_delete_by_content(matched_msg->sender, LPS[lid_receiver]->queue_in, matched_msg);
						//
					 	if (matched_msg->checkpoint_of_event != NULL) {
							matchd_msg->checkpoint_of_event->last_event = NULL;
						}	
							
						list_delete_by_content(lid_receiver, LPS[lid_receiver]->queue_in, matched_msg);
#endif
>>>>>>> origin/reverse

<<<<<<< HEAD
                        if (out_msg->mark == msg_to_process->mark) {
                            matched_out_msg = out_msg;
                            break;
                        }

						out_msg = list_prev(out_msg);
					}

					if(matched_out_msg == NULL) {

						if (has_cancelback_started())
							stylized_printf("Cancelback started!\n", CYAN, true);
						else
							stylized_printf("Cancelback did not start!\n", CYAN, true);

                        rootsim_error(false, "LP %d Received a positive Cancelback message with mark \033[1;31m%llu\033[0m and send time \033[1;34m%f\033[0m (current LVT \033[1;34m%f\033[0m) from LP %u, but no such mark found in the output queue!\n", LPS_bound[i]->lid, msg_to_process->mark, msg_to_process->send_time, lvt(lid_receiver), msg_to_process->sender);
						printf("Message Content:"
							"sender: %d\n"
							"receiver: %d\n"
							"type: %d\n"
							"timestamp: %f\n"
							"send time: %f\n"
							"mark: %llu\n"
							"rendezvous mark %llu\n",
							msg_to_process->sender,
							msg_to_process->receiver,
							msg_to_process->type,
							msg_to_process->timestamp,
							msg_to_process->send_time,
							msg_to_process->mark,
							msg_to_process->rendezvous_mark);
						fflush(stdout);
                        abort();

<<<<<<< HEAD
					} else {

						// printf("LP %u received Cancelback message \033[1;31m%llu\033[0m from LP %u\n", lid_receiver, msg_to_process->mark, msg_to_process->sender);

                        // If the matched message is in the past, we have to rollback
                        if(matched_out_msg->send_time <= lvt(lid_receiver)) {

                            while (LPS[lid_receiver]->bound != NULL && LPS[lid_receiver]->bound->timestamp > msg_to_process->send_time) {
                                LPS[lid_receiver]->bound = list_prev(LPS[lid_receiver]->bound);
                            }

							// printf("\033[1;35mPositive Cancelback message %llu causing LP %u to rollback\033[0m\n", msg_to_process->mark, LidToGid(lid_receiver));

                            LPS[lid_receiver]->state = LP_STATE_SYNCH_FOR_CANCELBACK;
							LPS[lid_receiver]->state_to_resume = LP_STATE_ROLLBACK;
                        }

                        // Delete the matched message
                        list_delete_by_content(LPS[lid_receiver]->queue_out, matched_out_msg);
                    }
=======
						if(matched_msg->unprocessed == false)
							goto delete;
=======
#ifdef HAVE_REVERSE
					msg_to_process->revwin = NULL;
#endif
					list_place_by_content(lid_receiver, LPS[lid_receiver]->queue_in, timestamp, msg_to_process);
>>>>>>> origin/reverse

						// Unchain the event from the input queue
						list_delete_by_content(receiver->queue_in, matched_msg);
						list_insert_tail(LPS(lid_receiver)->retirement_queue, matched_msg);

						// Rollback last sent time as well if needed
						if(receiver->bound->timestamp < LPS(lid_receiver)->last_sent_time)
							LPS(lid_receiver)->last_sent_time = receiver->bound->timestamp;

					} else {
					    delete:
						// Unchain the event from the input queue
						list_delete_by_content(receiver->queue_in, matched_msg);
						// Delete the matched message
						msg_release(matched_msg);
						//list_insert_tail(LPS(lid_receiver)->retirement_queue, matched_msg);
					}
>>>>>>> origin/power

					break;

                case positive:
>>>>>>> origin/cancelback

<<<<<<< HEAD
			    //spin_lock(&receiver->bound_lock);
=======
					// A positive message is directly placed in the queue
	//				list_insert(receiver->queue_in, timestamp, msg_to_process);
	do {\
		__typeof__(msg_to_process) __n; /* in-block scope variable */\
		__typeof__(msg_to_process) __new_n = (msg_to_process);\
		size_t __key_position = my_offsetof((receiver->queue_in), timestamp);\
		double __key;\
		size_t __size_before;\
		rootsim_list *__l;\
		do {\
			__l = (rootsim_list *)(receiver->queue_in);\
			assert(__l);\
			__size_before = __l->size;\
			if(__l->size == 0) { /* Is the list empty? */\
				__new_n->prev = NULL;\
				__new_n->next = NULL;\
				__l->head = __new_n;\
				__l->tail = __new_n;\
				break;\
			}\
			__key = get_key(__new_n); /* Retrieve the new node's key */\
			/* Scan from the tail, as keys are ordered in an increasing order */\
			__n = __l->tail;\
			while(__n != NULL && __key < get_key(__n)) {\
				__n = __n->prev;\
			}\
			/* Insert depending on the position */\
		 	if(__n == __l->tail) { /* tail */\
				__new_n->next = NULL;\
				((__typeof(msg_to_process))__l->tail)->next = __new_n;\
				__new_n->prev = __l->tail;\
				__l->tail = __new_n;\
			} else if(__n == NULL) { /* head */\
				__new_n->prev = NULL;\
				__new_n->next = __l->head;\
				((__typeof(msg_to_process))__l->head)->prev = __new_n;\
				__l->head = __new_n;\
			} else { /* middle */\
				__new_n->prev = __n;\
				__new_n->next = __n->next;\
				__n->next->prev = __new_n;\
				__n->next = __new_n;\
			}\
		} while(0);\
		__l->size++;\
		assert(__l->size == (__size_before + 1));\
	} while(0);

>>>>>>> origin/power

<<<<<<< HEAD
				statistics_post_data(receiver, STAT_ANTIMESSAGE, 1.0);

				matched_msg = list_tail(receiver->queue_in);    // Find the message matching the antimessage
				while (matched_msg != NULL && matched_msg->mark != msg_to_process->mark) {
					matched_msg = list_prev(matched_msg);
				}
=======
					// Check if we've just inserted an out-of-order event
					if(msg_to_process->timestamp < lvt(lid_receiver)) {
#ifdef HAVE_REVERSE
						if(LPS[lid_receiver]->old_bound == NULL)
							LPS[lid_receiver]->old_bound = LPS[lid_receiver]->bound;
#endif

						LPS[lid_receiver]->bound = list_prev(msg_to_process);
						while ((LPS[lid_receiver]->bound != NULL) && LPS[lid_receiver]->bound->timestamp == msg_to_process->timestamp) {
							LPS[lid_receiver]->bound = list_prev(LPS[lid_receiver]->bound);
						}

<<<<<<< HEAD
						if (LPS[lid_receiver]->state != LP_STATE_SYNCH_FOR_CANCELBACK) {
							LPS[lid_receiver]->state = LP_STATE_ROLLBACK;
						}
					}

                    break;
=======
						receiver->state = LP_STATE_ROLLBACK;

						// Rollback last sent time as well if needed
						if(receiver->bound->timestamp < LPS(lid_receiver)->last_sent_time)
							LPS(lid_receiver)->last_sent_time = receiver->bound->timestamp;
					}

					#ifdef HAVE_MPI
					register_incoming_msg(msg_to_process);
					#endif
					break;
>>>>>>> origin/power

<<<<<<< HEAD
				// It's a control message
				case other:

					// Check if it is an anti control message
					if(!anti_control_message(msg_to_process)) {
						goto expunge_msg;
					}

					break;
>>>>>>> origin/cancelback

				if (unlikely(matched_msg == NULL)) {    	// Sanity check
					rootsim_error(false,"LP %d Received an antimessage, but no such mark has been found!\n",
						      receiver->gid.to_int);
					dump_msg_content(msg_to_process);
					rootsim_error(true, "Aborting...\n");
				}

                ts_bound = receiver->bound->timestamp;  // If the matched message is in the past, we have to rollback
                if (matched_msg->timestamp <= ts_bound) {

                    receiver->bound = list_prev(matched_msg);
                    while ((receiver->bound != NULL) && D_EQUAL(receiver->bound->timestamp, msg_to_process->timestamp)) {
                        receiver->bound = list_prev(receiver->bound);
                        assert(receiver->bound != NULL);
                    }

                    receiver->state = LP_STATE_ROLLBACK;
                    receiver->rollback_status = REQUESTED;
                }

                list_delete_by_content(receiver->queue_in, matched_msg);  // The matched message is unchained from the queue in

                if(matched_msg->unprocessed == true || receiver->last_processed == matched_msg) {
                    // If the pointer is still reachable, give it to a garbage collector
                    list_insert_tail(receiver->retirement_queue, matched_msg);
                } else {
                    msg_release(matched_msg);  // Delete the matched message
                }
#ifdef HAVE_MPI
            register_incoming_msg(msg_to_process);
#endif
                //spin_unlock(&receiver->bound_lock);
				break;

				// It's a positive message
			    case positive:

                //spin_lock(&receiver->bound_lock);
				// A positive message is directly placed in the queue
				list_insert(receiver->queue_in, timestamp, msg_to_process);

                // Check if we've just inserted an out-of-order event.
				// Here we check for a strictly minor timestamp since
				// the queue is FIFO for same-timestamp events. Therefore,
				// A contemporaneous event does not cause a causal violation.

				ts_bound = receiver->bound->timestamp;  // bound has been NULL once
				if (msg_to_process->timestamp < ts_bound) {
                    if(!straggler_set){
                        straggler_set = true;       //for this particular LP belonging to THIS thread
                        stragglers_received += 1;
                    }

                    assert(list_prev(msg_to_process) != NULL);
					receiver->bound = list_prev(msg_to_process);
                    assert(receiver->bound != NULL);
                    while ((receiver->bound != NULL) && D_EQUAL(receiver->bound->timestamp, msg_to_process->timestamp)) {
						receiver->bound = list_prev(receiver->bound);
                        assert(receiver->bound != NULL);
					}

					receiver->state = LP_STATE_ROLLBACK;
                    receiver->rollback_status = REQUESTED;
                 }
				#ifdef HAVE_MPI
				register_incoming_msg(msg_to_process);
				#endif
                    //spin_unlock(&receiver->bound_lock);
                 break;

                // It's a control message
                case control:
                 // Check if it is an anti control message
                 if (!anti_control_message(msg_to_process)) {
                     msg_release(msg_to_process);
                     continue;
                 }

                 break;

             default:
                 rootsim_error(true, "Received a message which is neither positive nor negative. Aborting...\n");
             }
=======
				default:
					rootsim_error(true, "Received a message which is neither positive nor negative. Aborting...\n");
			}

>>>>>>> origin/reverse
		}
	}
<<<<<<< HEAD
	straggler_set = false;
    straggler_percentage = (stragglers_received / n_lp_per_thread) * 100;
    post_stragglers_percentage(straggler_percentage);
	stragglers_received = 0;

    // We have processed all in transit messages.
    // Actually, during this operation, some new in transit messages could
    // be placed by other threads. In this case, we loose their presence.
    // This is not a correctness error. The only issue could be that the
    // preemptive scheme will not detect this, and some events could
    // be in fact executed out of order.
    #ifdef HAVE_PREEMPTION
    reset_min_in_transit(local_tid);
    #endif
 }

 /**
 * This function generates a mark value that is unique w.r.t. the previous values for each Logical Process.
 * It is based on the Cantor Pairing Function, which maps 2 naturals to a single natural.
 * The two naturals are the LP gid (which is unique in the system) and a non decreasing number
 * which gets incremented (on a per-LP basis) upon each function call.
 * It's fast to calculate the mark, it's not fast to invert it. Therefore, inversion is not
 * supported at all in the simulator code (but an external utility is provided for debugging purposes,
 * which can be found in src/lp_mark_inverse.c)
 *
 * @author Alessandro Pellegrini
 *
 * @param lp A pointer to the LP's lp_struct for which we want to generate
 *           a system-wide unique mark
 * @return A value to be used as a unique mark for the message within the LP
 */
unsigned long long generate_mark(struct lp_struct *lp)
{
	unsigned long long k1 = (unsigned long long)lp->gid.to_int;
	unsigned long long k2 = lp->mark++;

	return (unsigned long long)(((k1 + k2) * (k1 + k2 + 1) / 2) + k2);
=======

	// We have processed all in transit messages.
	// Actually, during this operation, some new in transit messages could
	// be placed by other threads. In this case, we loose their presence.
	// This is not a correctness error. The only issue could be that the
	// preemptive scheme will not detect this, and some events could
	// be in fact executed out of order.
	#ifdef HAVE_PREEMPTION
	reset_min_in_transit(tid);
	#endif
>>>>>>> origin/power
}
