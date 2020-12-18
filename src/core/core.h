/**
 * @file core/core.h
 *
 * @brief Core ROOT-Sim functionalities
 *
 * Core ROOT-Sim functionalities
 *
 * @copyright
 * Copyright (C) 2008-2020 HPDCS Group
 * https://rootsim.github.io/core
 *
 * This file is part of ROOT-Sim (ROme OpTimistic Simulator).
 *
 * ROOT-Sim is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; only version 3 of the License applies.
 *
 * ROOT-Sim is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ROOT-Sim; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#pragma once

#include <ROOT-Sim.h>
#include <log/log.h>
#include <mm/mm.h>

#include <float.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef CACHE_LINE_SIZE // TODO: calculate and inject at build time
#define CACHE_LINE_SIZE 64
#endif

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
extern __thread jmp_buf exit_jmp_buf;

extern void ProcessEvent(lp_id_t me, simtime_t now, unsigned event_type,
	const void *content, unsigned size, void *state);
extern void ProcessEvent_pr(lp_id_t me, simtime_t now, unsigned event_type,
	const void *content, unsigned size, void *state);
extern bool CanEnd(lp_id_t me, const void *state);
