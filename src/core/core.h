#pragma once

#include <NeuRome.h>
#include <log/log.h>

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef CACHE_LINE_SIZE // TODO: calculate and inject at build time
#define CACHE_LINE_SIZE 64
#endif

#define max(a, b) 			\
__extension__({				\
	__typeof__ (a) _a = (a);	\
	__typeof__ (b) _b = (b);	\
	_a > _b ? _a : _b;		\
})


#define min(a, b) 			\
__extension__({				\
	__typeof__ (a) _a = (a);	\
	__typeof__ (b) _b = (b);	\
	_a < _b ? _a : _b;		\
})

/// Optimize the branch as likely taken
#define likely(exp) __builtin_expect(!!(exp), 1)
/// Optimize the branch as likely not taken
#define unlikely(exp) __builtin_expect(!!(exp), 0)

/// The type used to represent time in the simulation, we use fixed point numbers
typedef double simtime_t;
//  FIXME
#define MAX_SIMTIME_T 1000

#define lp_id_to_lid(lp_id) ((lp_id) & ((UINT64_C(1) << UINT64_C(48)) - 1U))
#define lp_id_to_nid(lp_id) ((unsigned)((lp_id) >> UINT64_C(48)))
#define nid_lid_to_lp_id(nid, lid) (((nid) << UINT64_C(48)) | (lid))

/// The type used to uniquely identify LPs in the simulation
typedef uint64_t lp_id_t;
typedef unsigned rid_t;
typedef unsigned nid_t;

extern uint64_t n_lps;
extern unsigned n_threads;
extern unsigned n_nodes;
extern __thread rid_t rid;
extern nid_t nid;


extern void core_init(void);

extern void ProcessEvent(unsigned me, simtime_t now, unsigned event_type,
	const void *content, unsigned size, void *state);
extern bool OnGVT(unsigned me, const void *state);
