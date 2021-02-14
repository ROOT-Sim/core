/**
 * @file core/core.h
 *
 * @brief Core ROOT-Sim functionalities
 *
 * Core ROOT-Sim functionalities
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <log/log.h>
#include <mm/mm.h>

#include <float.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef max
#undef max
#endif
#define max(a, b) 			\
__extension__({				\
	__typeof__ (a) _a = (a);	\
	__typeof__ (b) _b = (b);	\
	_a > _b ? _a : _b;		\
})

#ifdef min
#undef min
#endif
#define min(a, b) 			\
__extension__({				\
	__typeof__ (a) _a = (a);	\
	__typeof__ (b) _b = (b);	\
	_a < _b ? _a : _b;		\
})

#ifndef CACHE_LINE_SIZE // TODO: calculate and inject at build time
/// The size of a cpu cache line
/** Used to align some data structures in order to avoid false sharing */
#define CACHE_LINE_SIZE 64
#endif

#define INIT 0
#define DEINIT UINT_MAX

/// Optimize the branch as likely taken
#define likely(exp) __builtin_expect(!!(exp), 1)
/// Optimize the branch as likely not taken
#define unlikely(exp) __builtin_expect((exp), 0)

/// The type used to represent logical time in the simulation
typedef double simtime_t;
/// The maximum value of the logical simulation time, semantically never
#define SIMTIME_MAX DBL_MAX

/// The maximum number of supported MPI nodes
/** FIXME: this is used very limitedly. Consider its removal */
#define MAX_NODES (1 << 16)
/// The maximum number of supported threads
/** FIXME: this is used very limitedly. Consider its removal */
#define MAX_THREADS (1 << 10)
#define lid_to_nid(lid) (nid_t)(lid / n_lps_node)

/// Used to uniquely identify LPs in the simulation
typedef uint64_t lp_id_t;
/// Used to identify in a node the computing resources (threads at the moment)
typedef unsigned rid_t;
/// Used to identify MPI nodes in a distributed environment
typedef int nid_t;

/// The total number of LPs in the simulation
extern lp_id_t n_lps;
/// The total number of LPs hosted in the node
extern lp_id_t n_lps_node;
/// The total number of MPI nodes in the simulation
extern rid_t n_threads;
/// The identifier of the thread
extern __thread rid_t rid;

#ifdef ROOTSIM_MPI

extern nid_t n_nodes;
/// The node identifier of the node
extern nid_t nid;
/// The total number of threads running in the node

#else
enum {nid = 0, n_nodes = 1};
#endif

extern void ProcessEvent(lp_id_t me, simtime_t now, unsigned event_type,
	const void *content, unsigned size, void *state);
extern void ProcessEvent_pr(lp_id_t me, simtime_t now, unsigned event_type,
	const void *content, unsigned size, void *state);
extern bool CanEnd(lp_id_t me, const void *state);
