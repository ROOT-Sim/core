/**
 * @file arch/atomic.h
 *
 * @brief Atomic operations
 *
 * This module implements atomic and non-blocking operations used
 * within ROOT-Sim to coordinate threads and processes (on shared memory).
 *
 * @copyright
 * Copyright (C) 2008-2019 HPDCS Group
 * https://hpdcs.github.io
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
 *
 * @author Alessandro Pellegrini
 *
 * @date Jan 25, 2012
 */

#pragma once

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

/// This is the definition of a software barrier. No effect on hardware
/// accesses comes from the usage of this instruction!
#define barrier() __asm__ __volatile__("":::"memory")

#if defined(__x86_64__)
  #define cpu_relax() __asm__ __volatile__("pause": : :"memory")
#else
  #define cpu_relax()
#endif

/// RMW instructions used in the engine
#define cmpxchg(P, O, N) atomic_compare_exchange_strong_explicit((P), (O), (N), memory_order_acq_rel, memory_order_acquire)
#define __atomic_add(P, V) atomic_fetch_add_explicit((P), (V), memory_order_release)
#define __atomic_sub(P, V) atomic_fetch_sub_explicit((P), (V), memory_order_release)


<<<<<<< HEAD
inline bool iCAS(volatile uint32_t * ptr, uint32_t oldVal, uint32_t newVal);
inline int atomic_test_and_set(int *);
inline void atomic_inc(atomic_t *);
inline void atomic_dec(atomic_t *);
inline int atomic_inc_and_test(atomic_t * v);
inline bool spin_trylock(spinlock_t * s);
inline void spin_unlock(spinlock_t * s);
inline void spin_lock(spinlock_t * s);
inline void atomic_add(atomic_t *, int);

<<<<<<< HEAD
=======
#define SPINLOCK_GIVES_COUNT



#if defined(ARCH_X86) || defined(ARCH_X86_64)

/// Atomic counter definition
typedef struct { volatile int count; } atomic_t;

/// Spinlock definition
typedef struct { volatile unsigned int lock; } spinlock_t;

<<<<<<< HEAD


extern inline bool CAS_x86(volatile unsigned long long *ptr, unsigned long long oldVal, unsigned long long newVal);
extern inline bool iCAS_x86(volatile unsigned int *ptr, unsigned int oldVal, unsigned int newVal);
extern inline int atomic_test_and_set_x86(int *);
extern inline int atomic_test_and_reset_x86(int *);
extern inline void atomic_add_x86(atomic_t *, int);
extern inline void atomic_sub_x86(atomic_t *, int);
extern inline void atomic_inc_x86(atomic_t *);
extern inline void atomic_dec_x86(atomic_t *);
extern inline int atomic_inc_and_test_x86(atomic_t *v);
extern inline bool spin_trylock_x86(spinlock_t *s);
extern inline void spin_unlock_x86(spinlock_t *s);
=======
#define CAS(ptr, old, new) __sync_bool_compare_and_swap((ptr), (old), (new))
inline bool iCAS(volatile uint32_t *ptr, uint32_t oldVal, uint32_t newVal);
inline int atomic_test_and_set(int *);
inline int atomic_test_and_reset(int *);
inline void atomic_add(atomic_t *, int);
inline void atomic_sub(atomic_t *, int);
inline void atomic_inc(atomic_t *);
inline void atomic_dec(atomic_t *);
inline int atomic_inc_and_test(atomic_t *v);
inline bool spin_trylock(spinlock_t *s);
inline void spin_unlock(spinlock_t *s);
>>>>>>> origin/power

#ifdef SPINLOCK_GIVES_COUNT
extern inline unsigned int spin_lock_x86(spinlock_t *s);
#else
extern inline void spin_lock_x86(spinlock_t *s);
#endif


#define CAS			CAS_x86
#define iCAS			iCAS_x86
#define atomic_test_and_set	atomic_test_and_set_x86
#define atomic_test_and_reset	atomic_test_and_reset_x86
#define atomic_add		atomic_add_x86
#define atomic_sub		atomic_sub_x86
#define atomic_dec		atomic_dec_x86
#define atomic_inc		atomic_inc_x86
#define atomic_inc_and_test	atomic_inc_and_test_x86
#define spin_lock		spin_lock_x86
#define spin_trylock		spin_trylock_x86
#define spin_unlock		spin_unlock_x86
>>>>>>> origin/cancelback

#define LOCK "lock; "

=======
/// Atomic type definition
typedef atomic_int atomic_t;
#define atomic _Atomic

#define atomic_add(P, V)	__atomic_add((P), (V))
#define atomic_inc(P)		atomic_add((P), 1)
#define atomic_sub(P, V)	__atomic_sub((P), (V))
#define atomic_dec(P)		atomic_sub((P), 1)
>>>>>>> origin/atomic

/// Read operation on an atomic counter
#define atomic_read(P)		atomic_load_explicit((P), memory_order_acquire)

/// Set operation on an atomic counter
#define atomic_set(P, V)	atomic_init((P), (V))


/**
 * Spinlock definition. We use a ticket lock here. The ticket lock
 * can be extremely slow when the number of threads exceeds the
 * number of CPUs. This is something which we explicitly forbid in
 * ROOT-Sim, so this is the locking strategy which we use. Indeed,
 * fairness is quite important here.
 */
typedef union ticketlock spinlock_t;
union ticketlock {
    unsigned u;
    struct {
        unsigned short ticket;
        unsigned short users;
    } s;
};

/// Spinlock initialization
<<<<<<< HEAD
#define plain_spinlock_init(s)	((s)->lock = 0)
=======
#define spinlock_init(P)	((P)->u = 0)


static inline void spin_lock(spinlock_t *t) {
    unsigned short me = atomic_add(&t->s.users, 1);

    while (t->s.ticket != me)
	cpu_relax();
}

static inline void spin_unlock(spinlock_t *t) {
    barrier();
    t->s.ticket++;
}

static inline bool spin_trylock(spinlock_t *t) {
    unsigned short me = t->s.users;
    unsigned short menew = me + 1;
    unsigned cmp = ((unsigned) me << 16) + me;
    unsigned cmpnew = ((unsigned) menew << 16) + me;

    if (cmpxchg(&t->u, &cmp, cmpnew))
	return true;

    return false;
}
>>>>>>> origin/atomic

 #define spinlock_init(s)	plain_spinlock_init(s) 