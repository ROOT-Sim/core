#pragma once

#include <lib/random/random.h>
//#include <lib/topology/topology.h>

struct lib_state_managed {
	// random library
	uint128_t rng_state;
};

struct lib_state {
	// topology
};

#define lib_checkpoint() __extension__({/*TODO*/})
#define lib_restore() __extension__({/*TODO*/})
#define lib_global_init() __extension__({/*TODO*/})
#define lib_global_fini() __extension__({/*TODO*/})
#define lib_init(state, lp_id) __extension__({/*TODO*/})
#define lib_fini(state) __extension__({/*TODO*/})
