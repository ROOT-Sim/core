#pragma once

#include <datatypes/bitmap.h>



/// this is used to store the common characteristics of the topology
extern struct _topology_global_t {
	unsigned directions;			/**< the number of valid directions in the topology */
	unsigned edge; 				/**< the pre-computed edge length (if it makes sense for the current topology geometry) */
	unsigned regions_cnt; 			/**< the number of LPs involved in the topology */
	enum _topology_geometry_t geometry;	/**< the topology geometry (see ROOT-Sim.h) */
} topology_global;

// this initializes the topology environment
void topology_init(void);

//used internally (also in abm_layer module) to schedule our reserved events TODO: move in a more system-like module
void UncheckedScheduleNewEvent(unsigned int gid_receiver, simtime_t timestamp, unsigned int event_type, void *event_content, unsigned int event_size);

// if the model is using a topology this gets called instead of the plain ProcessEvent
void ProcessEventTopology(void);
