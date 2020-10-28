#pragma once

#include <NeuRome.h>
#include <log/log.h>
#include <log/stats.h>
#include <mm/mm.h>

#include <assert.h>
#include <float.h>
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

/// The type used to represent time in the simulation
typedef double simtime_t;
#define SIMTIME_MAX DBL_MAX

#define MAX_NODES_BITS (16)
#define MAX_THREADS_BITS (10)
#define lid_to_nid(lid) (nid_t)(lid / n_lps_node)

/// The type used to uniquely identify LPs in the simulation
typedef uint64_t lp_id_t;
typedef unsigned rid_t;
typedef int nid_t;

extern lp_id_t n_lps;
extern lp_id_t n_lps_node;

extern nid_t n_nodes;
extern nid_t nid;

extern rid_t n_threads;
extern __thread rid_t rid;

extern void core_init(void);
extern void core_abort(void);

extern void ProcessEvent(lp_id_t me, simtime_t now, unsigned event_type,
	const void *content, unsigned size, void *state);
extern void ProcessEvent_instr(lp_id_t me, simtime_t now, unsigned event_type,
	const void *content, unsigned size, void *state);
extern bool CanEnd(lp_id_t me, const void *state);
