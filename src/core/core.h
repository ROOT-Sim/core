#pragma once

#include <NeuRome.h>
#include <log/log.h>

#include <stdint.h>
#include <stdbool.h>

#ifndef CACHE_LINE_SIZE // TODO: calculate and inject at build time
#define CACHE_LINE_SIZE 64
#endif
/// Correct way to inform the compiler of a cast
#define UNION_CAST(x, destType) (((union {__typeof__(x) a; destType b;})x).b)

#define max(a, b) 				\
	__extension__({				\
		__typeof__ (a) _a = (a);	\
		__typeof__ (b) _b = (b);	\
		_a > _b ? _a : _b;		\
	})

/// Optimize the branch as likely taken
#define likely(exp) __builtin_expect(!!(exp), 1)
/// Optimize the branch as likely not taken
#define unlikely(exp) __builtin_expect(!!(exp), 0)
/// The type used to uniquely identify LPs in the simulation
typedef uint64_t lp_id_t;
/// The type used to represent time in the simulation, we use fixed point numbers
typedef double simtime_t;

#ifndef NEUROME_SERIAL
extern __thread unsigned tid;
extern void core_thread_id_assign(void);
#endif

extern void ProcessEvent(unsigned me, simtime_t now, unsigned event_type,
	const void *content, unsigned size, void *state);
extern bool OnGVT(unsigned me, void *state);
