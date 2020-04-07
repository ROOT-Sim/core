#pragma once

#include <NeuRome.h>

#include <argp.h>

#include <lib/random/random.h>
#include <lib/topology/topology.h>
#include <lib/state/state.h>

extern const struct argp lib_argp;

struct lib_state_managed {
	// random library
	uint64_t rng_s[4];
	// state library
	void *state_s;
};

struct lib_state {
	// topology
};

#define lib_checkpoint()			\
	__extension__({				\
	})

#define lib_restore()				\
	__extension__({				\
	})

#define lib_global_init()			\
	__extension__({				\
		topology_init();		\
	})

#define lib_global_fini()			\
	__extension__({				\
	})

#define lib_init(state, lp_id) 			\
	__extension__({				\
						\
	})

#define lib_fini(state)				\
	__extension__({				\
	})
