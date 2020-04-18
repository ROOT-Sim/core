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
	unsigned fake_member;
	// topology
};

extern void lib_global_init(void);
extern void lib_global_fini(void);

extern void lib_lp_init(void);
extern void lib_lp_fini(void);
