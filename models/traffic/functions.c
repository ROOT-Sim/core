/**
*
* TRAFFIC is a simulation model for the ROme OpTimistic Simulator (ROOT-Sim)
* which allows to simulate car traffic on generic routes, which can be
* specified from text file.
*
* The software is provided as-is, with no guarantees, and is released under
* the GNU GPL v3 (or higher).
*
* For any information, you can find contact information on my personal webpage:
* http://www.dis.uniroma1.it/~pellegrini
*
* @file functions.c
* @brief Implementation of functions for supporting the simulation
* @author Alessandro Pellegrini
* @date January 12, 2012
*/

#include <ROOT-Sim.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "application.h"
#include "normal_cdf.h"

// bubble sort Q based on leave time. Then update travelled
static car_t *reorder_queue(car_t *head, simtime_t now, lp_state_type *state) {
    car_t *curr;
    car_t *prev;
    bool didSwap = false;
    for(didSwap = true; didSwap; ) {
        didSwap = false;
        prev = head;
        for(curr = head; (curr != NULL && curr->next != NULL); curr = curr->next) {
                if(curr->leave > curr->next->leave) {
                        if (head == curr) {
                            head = curr->next;      
                            curr->next = head->next; 
                            head->next = curr; 
                            //prev = head;
                        } else {
                            prev->next = curr->next;
                            curr->next = prev->next->next;
                            prev->next->next = curr;
                        }
                        didSwap = true;
                } 
                
                //curr->speed = abs(Gaussian(state->segment_length/(curr->leave - curr->arrival), SPEED_SIGMA)); //TODO
                //else if (head != curr) {
                //    prev = prev->next; //<- verifica
                //}
                prev = curr;
                
        }
    }


    // Update the portion of traveled space
	curr = head;
	while(curr != NULL) {
		curr->traveled = (now - curr->arrival) / (curr->leave - curr->arrival);
		curr = curr->next;
	}
    return head;
}


unsigned long long get_mark(unsigned int k1, unsigned int k2) {
	return (unsigned long long)( ((k1 + k2) * (k1 + k2 + 1) / 2) + k2 );
}


/*static*/ double Gaussian(double m, double s)
/* ========================================================================
 * Returns a normal (Gaussian) distributed real number.
 * NOTE: use s > 0.0
 *
 * Uses a very accurate approximation of the normal idf due to Odeh & Evans,
 * J. Applied Statistics, 1974, vol 23, pp 96-97.
 * ========================================================================
 * copied from http://www.cs.wm.edu/~va/software/park/
 */
{
	const double p0 = 0.322232431088;     const double q0 = 0.099348462606;
	const double p1 = 1.0;                const double q1 = 0.588581570495;
	const double p2 = 0.342242088547;     const double q2 = 0.531103462366;
	const double p3 = 0.204231210245e-1;  const double q3 = 0.103537752850;
	const double p4 = 0.453642210148e-4;  const double q4 = 0.385607006340e-2;
	double u, t, p, q, z;

	u = Random();
	if (u < 0.5)
		t = sqrt(-2.0 * log(u));
	else
		t = sqrt(-2.0 * log(1.0 - u));
	p = p0 + t * (p1 + t * (p2 + t * (p3 + t * p4)));
	q = q0 + t * (q1 + t * (q2 + t * (q3 + t * q4)));

	if (u < 0.5)
		z = (p / q) - t;
	else
		z = t - (p / q);

	return (m + s * z);
}


static simtime_t compute_traverse_time(lp_state_type *state, double mean_speed, car_t *car) {
	double traffic;
	double speed, real_speed;
	simtime_t traverse_time;

	// Compute the traffic scaling factor
	traffic = (double)state->queued_elements / (double)state->total_queue_slots;

	// Compute the speed
	speed = MIN_SPEED;
	if(mean_speed > MIN_SPEED) {
		speed += (mean_speed - MIN_SPEED) * (1 - traffic);
	}

	do {
		// Compute traverse time according to a Normal distribution
		real_speed = fabs(Gaussian(speed, SPEED_SIGMA));
		car->speed = real_speed;
		traverse_time = (simtime_t)(state->segment_length / real_speed);
	} while(isinf(traverse_time));

	// Convert the traverse time in seconds
	traverse_time *= 3600.0;

	return traverse_time;
}


void release_cars(unsigned int me, lp_state_type *state) {
	car_t *curr_car;
	//simtime_t leave_time;
	(void) me;
	
	curr_car = state->queue;
	while(curr_car != NULL) {
		if(curr_car->accident == true) {
			curr_car->accident = false;
		}
		
		curr_car = curr_car->next;
	}
}

car_t *car_enqueue(int me, int from, lp_state_type *state) {
	car_t *new_car;
	//car_t *curr_car;
	
	// Create the car node
	new_car = malloc(sizeof(car_t));
	bzero(new_car, sizeof(car_t));
	new_car->from = from;
	new_car->arrival = state->lvt;
	new_car->leave = state->lvt /*+LOOKAHEAD*/ + compute_traverse_time(state, AVERAGE_SPEED, new_car);
	new_car->car_id = get_mark(me, state->car_id++);
	if(state->accident)
		new_car->accident = true;
	
	state->queued_elements++;
/*
	if(state->queue == NULL) {
		state->queue = new_car;
		return new_car->leave;
	}
	
	if(state->queue->leave < new_car->leave) {
		new_car->next = state->queue;
		state->queue = new_car;
		return new_car->leave;
	}
	
	// Insert the car in reverse time order
	curr_car = state->queue;
	while(curr_car->next != NULL && curr_car->next->leave > new_car->leave)
		curr_car = curr_car->next;
	
	new_car->next = curr_car->next;
	curr_car->next = new_car;
*/

	new_car->next = state->queue;
	state->queue = new_car;
	
	//~printf("\n%d: Enqueueing %llu: ", me, new_car->car_id);
//	curr_car = state->queue;
//	while(curr_car != NULL) {
		//~printf("%llu, ", curr_car->car_id);
//		curr_car = curr_car->next;
//	}

	state->queue = reorder_queue(state->queue, state->lvt, state);
#if VERBOSE > 2
	printf("ENQUEUE: car %llu entering in LP %u- at time %f. It will depart at time %f\n", new_car->car_id, me, new_car->arrival, new_car->leave);
#endif	
	return new_car;
}

// Decide when a new car will join the simulation through the current junction
void inject_new_cars(lp_state_type *state, int me) {
	simtime_t timestamp;
	event_content_type new_evt;

	// If the node has an interarrival frequency == -1, then no car can enter the node (i.e., a junction
	// with no actual ramps)
	if(D_EQUAL(state->enter_prob, -1.0)) {
		return;
	}

	// Entering timestamps ditributed according to an Erlang distribution
	timestamp = state->lvt + (simtime_t)(Expent(state->enter_prob /*-LOOKAHEAD*/))/*+LOOKAHEAD*/;
	// Send me the inject event
	new_evt.from = me;
	new_evt.injection = 1;
	ScheduleNewEvent(me, timestamp, ARRIVAL, &new_evt, sizeof(event_content_type));
}


// With a probability, cause an accident encompassing a random number of the trailing cars
void cause_accident(lp_state_type *state, int me) {
	double min;
	double max;
	double mean;
	double var;
	double prob;
	double coin;
	int involved_car;
	int i;
	car_t *curr_car;
	simtime_t duration;
	
	// if there is already an accident, don't cause another one
	if(state->accident) {
		return;
	}

	if(state->lp_type != JUNCTION)
		return;

	// An accident happens depending on the number of cars.
	// Accident probability is normal wrt the number of cars.
	// Derive a discrete probability from a normal distribution
	if(state->queued_elements == 0) {
		min = 0.0;
		max = 0.5;
	} else if(state->queued_elements == state->total_queue_slots) {
		min = (double)state->total_queue_slots - 0.5;
		max = (double)state->total_queue_slots;
	} else {
		min = (double)state->queued_elements - 0.5;
		max = (double)state->queued_elements + 0.5;
	}

	// TODO come calcolare qui la varianza?!
	mean = (double)state->total_queue_slots / 3.0; // When there are many cars but not that much, accidents are more likely to occur
	var = RandomRange(0, 100);

	
	prob = contourcdf(min, max, mean, var); // <- TORNA SEMPRE 0
	
	prob *= (double)ACCIDENT_PROBABILITY;

	// Toss a coin to check whether an accident occured or not
	coin = Random();
	
	// If there is an accident, set the parameters accordingly and determine how long the accident will last
	if(coin <= prob) {

		state->accident = true;

		// Compute when the road will be freed, according to the node's accident duration
		do {
			duration = (simtime_t)(Gaussian(ACCIDENT_DURATION, ACCIDENT_SIGMA));
		} while(duration <= 0);
		
		ScheduleNewEvent(me, /*LOOKAHEAD+*/ state->lvt + duration, FINISH_ACCIDENT, NULL, 0);

		//~printf("(%d) Accident at node %s at time %f, until %f\n", me, state->name, state->lvt, state->lvt + duration);
		
		// Select cars involved in the accident
		involved_car = RandomRange(0, state->queued_elements - 1);
		curr_car = state->queue;
		i = 0;
		while(i < involved_car) {
			curr_car = curr_car->next;
			i++;
		}
		
		while(curr_car != NULL) {
			curr_car->accident = true;
			curr_car = curr_car->next;
		}
	}
}



// Decide if a car will leave the simulation through current JUNCTION
int check_car_leaving(lp_state_type *state, int from, int me) {
	double coin;

	// If the node has a leaving probability == -1, then no car can leave the node
	if(D_EQUAL(state->leave_prob, -1.0)) {
		return 0;
	}

	if(state->lp_type == JUNCTION) {

		// If this is an end node and the car did not join the highway from here...
		if(state->topology->num_neighbours == 1 && from != me) {
			// Then the car has left
			return 1;
		}

		// If the car arrived here (and did not join from here) then check if it's leaving
		if(from != me) {
			coin = Random();
			if(coin <= state->leave_prob) {
				// The car has left
				return 1;
			}
		}
	}

	return 0;

}

// Find car with ID *mark and pop it. Update speeds of preceding cars
car_t *car_dequeue(unsigned int me, lp_state_type *state, unsigned long long *mark) {
	car_t *curr_car;
	car_t *ret_car;
	
	//~printf("\n%d: looking for %llu... ", me, *mark);
#if VERBOSE > 2	
	printf("DEQUEUE: car %llu outgoing at LP %u- at time %f. addr: %p\n", mark[0], me, state->lvt, mark);
#endif
	curr_car = state->queue;
	
	if(curr_car == NULL) {
		printf("ERROR_1: car %llu not found in LP %u\n", *mark, me);
		//abort();
		return NULL;
	}
	
	if(curr_car->car_id == *mark) {
		if(curr_car->accident || curr_car->stopped) {
			return NULL;
		}

		state->queue = curr_car->next;
		state->queued_elements--;
		return curr_car;
	}
	
	while(curr_car->next != NULL && curr_car->next->car_id != *mark) {
		//~printf("%llu, ", curr_car->next->car_id);
		curr_car = curr_car->next;
		curr_car->speed = fabs(Gaussian(state->segment_length/(curr_car->leave - curr_car->arrival), SPEED_SIGMA)); //TODO
	}
	
	if(curr_car->next == NULL) {
		//while(curr_car->next != NULL && curr_car->next->car_id != *mark) {
		//	printf("%llu, ", curr_car->next->car_id);
		//	curr_car = curr_car->next;
		//}
		printf("ERROR_2: car %llu not found in LP %u\n", *mark, me);
		//abort();
		return NULL;
	}
	
	//~printf("%llu, ", curr_car->next->car_id);
	
	ret_car = curr_car->next;
	if(ret_car->accident || ret_car->stopped) {
		return NULL;
	}

	curr_car->next = curr_car->next->next;
	
	state->queued_elements--;
	
	return ret_car;
}

// For each car, flip a coin and maybe stop it.
void determine_stop(lp_state_type *state) {
	car_t *car;
	double coin;

	car = state->queue;

	while(car != NULL) {
		coin = Random();
		if(coin < STOP_PROBABILITY) {
			car->stopped = true;
			car->traveled = (car->leave - car->arrival) / car->leave;
		}
		car = car->next;
	}
}

// update leave time of car with id ID. Reorder Q by car leave time
void update_car_leave(lp_state_type *state, unsigned long long id, simtime_t new) {
	car_t *curr_car = state->queue;
	
	while(curr_car != NULL) {
		if(curr_car->car_id == id) {
			curr_car->stopped = false;
			curr_car->leave = new;
			break;
		}
		curr_car = curr_car->next;
	}

	state->queue = reorder_queue(state->queue, state->lvt, state);
}
