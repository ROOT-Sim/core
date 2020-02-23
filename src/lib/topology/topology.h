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
