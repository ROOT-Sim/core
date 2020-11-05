/**
* @file datatypes/remote_msg_map.c
*
* @brief Message map datatype
*
* Message map datatype
*
* @copyright
* Copyright (C) 2008-2020 HPDCS Group
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
*/
#include <datatypes/remote_msg_map.h>

#include <core/intrinsics.h>
#include <core/sync.h>
#include <gvt/gvt.h>
#include <lp/msg.h>
#include <mm/mm.h>

#include <memory.h>

#define HM_INITIAL_CAPACITY 8
#define SWAP_VALUES(a, b) do {__typeof(a) _tmp = (a); (a) = (b); (b) = _tmp;} while(0)
#define MAX_LF 0.95
#define HB_LCK ((uintptr_t)1U)

typedef uint_fast32_t map_size_t;

struct msg_map_node_t {
	_Atomic(uintptr_t) msg_id;
	nid_t msg_nid;
	simtime_t until;
	struct lp_msg *msg;
};

#define inner_map {							\
	struct msg_map_node_t *nodes;					\
	map_size_t capacity_mo;						\
	atomic_int count;						\
}

static struct msg_map_t {
	struct inner_map;
	char padding[CACHE_LINE_SIZE - sizeof(struct inner_map)];
	struct {
		spinlock_t l;
		char l_padding[CACHE_LINE_SIZE - sizeof(spinlock_t)];
	} locks[1 << MAX_THREADS_BITS];
} re_map;

static __attribute__((const)) inline map_size_t msg_id_hash(uintptr_t msg_id)
{
	return (msg_id ^ (msg_id >> 32)) * 0xbf58476d1ce4e5b9ULL; // @suppress("Avoid magic numbers")
}

void remote_msg_map_global_init(void)
{
	map_size_t cnt = n_threads * 2 / (1 - MAX_LF);
	map_size_t cap = 1ULL << (sizeof(cnt) * CHAR_BIT - SAFE_CLZ(cnt));
	// capacity_mo is in the form 2^n - 1, modulo computations are then easy
	re_map.capacity_mo = cap - 1;
	atomic_store_explicit(&re_map.count, cap * MAX_LF, memory_order_relaxed);
	re_map.nodes = mm_alloc(sizeof(*re_map.nodes) * cap);
	memset(re_map.nodes, 0, sizeof(*re_map.nodes) * cap);
}

void remote_msg_map_global_fini(void)
{
	mm_free(re_map.nodes);
}

inline static void remote_msg_map_lock_all(void)
{
	const rid_t t_cnt = n_threads;
	bool done[t_cnt];
	memset(done, 0, sizeof(done));

	static spinlock_t ll;

	spin_lock(&ll);
	for (rid_t i = 0, r = t_cnt; r; i = (i + 1) % t_cnt) {
		if(done[i] || !spin_trylock(&re_map.locks[i].l))
			continue;

		done[i] = true;
		--r;
	}
	spin_unlock(&ll);
}

inline static void remote_msg_map_unlock_all(void)
{
	const rid_t t_cnt = n_threads;
	for (rid_t i = 0; i < t_cnt; ++i) {
		spin_unlock(&re_map.locks[i].l);
	}
}

void remote_msg_map_fossil_collect(simtime_t current_gvt)
{
	static struct msg_map_node_t *old_nodes = NULL;
	static atomic_int re_map_bar = 0;
	static map_size_t cap;

	if (!atomic_fetch_add_explicit(&re_map_bar, 1U, memory_order_acquire)) {
		remote_msg_map_lock_all();
		mm_free(old_nodes);
		old_nodes = re_map.nodes;
		cap = re_map.capacity_mo + 1;
		atomic_fetch_sub_explicit(&re_map_bar, n_threads,
			memory_order_release);

		re_map.nodes = mm_alloc(sizeof(struct msg_map_node_t) * cap);
		memset(re_map.nodes, 0, sizeof(struct msg_map_node_t) * cap);
		atomic_store_explicit(&re_map.count, cap * MAX_LF,
			memory_order_relaxed);
		remote_msg_map_unlock_all();
	} else {
		while (atomic_load_explicit(&re_map_bar,
			memory_order_acquire) > 0)
			spin_pause();
	}

	bool big = (cap % n_threads) > rid;

	map_size_t cnt = cap / n_threads;
	map_size_t off;
	if (big) {
		++cnt;
		off = cnt * rid;
	} else {
		off = cap - cnt * (n_threads - rid);
	}

	while (cnt--) {
		struct msg_map_node_t *node = &old_nodes[off + cnt];
		if (node->msg_id && current_gvt <= node->until)
			remote_msg_map_match(node->msg_id, node->msg_nid,
				node->msg);
	}
}

static void msg_map_size_increase(void)
{
	const map_size_t old_cmo = re_map.capacity_mo;
	const map_size_t cmo = old_cmo * 2 + 1;
	struct msg_map_node_t *old_nodes = re_map.nodes;
	struct msg_map_node_t *nodes = mm_alloc(sizeof(*nodes) * (cmo + 1));

	memset(nodes, 0, sizeof(*nodes) * (cmo + 1));

	remote_msg_map_lock_all();

	atomic_fetch_add_explicit(&re_map.count, old_cmo * MAX_LF,
		memory_order_release);

	for (map_size_t j = 0; j <= old_cmo; ++j) {
		if(!old_nodes[j].msg_id)
			continue;

		struct msg_map_node_t *cnode = &old_nodes[j];
		map_size_t cdib = 0;
		map_size_t i = msg_id_hash(atomic_load_explicit(&cnode->msg_id,
			memory_order_relaxed)) & cmo;

		while (nodes[i].msg_id) {
			map_size_t tdib = (cmo + 1 + i - (msg_id_hash(
				atomic_load_explicit(&nodes[i].msg_id,
				memory_order_relaxed)) & cmo)) & cmo;

			if (cdib > tdib) {
				cdib = tdib;
				struct msg_map_node_t tmp_node;
				memcpy(&tmp_node, cnode, sizeof(*cnode));
				memcpy(cnode, &nodes[i], sizeof(*cnode));
				memcpy(&nodes[i], &tmp_node, sizeof(*cnode));
			}
			i = (i + 1) & cmo;
			++cdib;
		}
		memcpy(&nodes[i], cnode, sizeof(*cnode));
	}
	re_map.capacity_mo = cmo;
	re_map.nodes = nodes;

	remote_msg_map_unlock_all();

	mm_free(old_nodes);
}

void remote_msg_map_match(uintptr_t msg_id, nid_t nid, struct lp_msg *msg)
{
	msg_id &= ~HB_LCK;
	map_size_t cdib = 0;
	struct lp_msg *cmsg = msg;
	simtime_t cuntil = msg ? msg->dest_t : SIMTIME_MAX;
	nid_t cnid = nid;
	uintptr_t cd = msg_id | HB_LCK;
	map_size_t i = msg_id_hash(msg_id);

	spinlock_t *lck = &re_map.locks[rid].l;
	spin_lock(lck);

	struct msg_map_node_t *n = re_map.nodes;
	const map_size_t cmo = re_map.capacity_mo;
	i &= cmo;
	// linear probing with robin hood hashing
	// https://cs.uwaterloo.ca/research/tr/1986/CS-86-14.pdf by Pedro Celis
	while (1) {
		uint64_t td = atomic_load_explicit(&n[i].msg_id,
			memory_order_relaxed);

		retry_zero_check:
		if (!td) {
			if (unlikely(!atomic_compare_exchange_weak_explicit(
				&n[i].msg_id, &td, cd, memory_order_acquire,
				memory_order_relaxed))) {
				goto retry_zero_check;
			}
			n[i].msg = cmsg;
			n[i].until = cuntil;
			n[i].msg_nid = cnid;
			atomic_fetch_sub_explicit(&n[i].msg_id, HB_LCK,
				memory_order_release);

			int c = atomic_fetch_add_explicit(&re_map.count, -1,
				memory_order_relaxed);

			spin_unlock(lck);

			if (unlikely(c <= 0)) {
				if (c) {
					while (atomic_load_explicit(
						&re_map.count,
						memory_order_acquire) <= 0);
				} else {
					msg_map_size_increase();
				}
			}
			return;
		}
		retry_eq_check:
		td &= ~HB_LCK;
		if (td == (cd & ~HB_LCK)) {
			if (unlikely(!atomic_compare_exchange_weak_explicit(
				&n[i].msg_id, &td, cd,
				memory_order_acquire, memory_order_relaxed))) {
				goto retry_eq_check;
			}

			if (likely(cnid == n[i].msg_nid)){
				if (likely(!cmsg))
					cmsg = n[i].msg;

				n[i].until = 0;

				atomic_fetch_sub_explicit(&n[i].msg_id, HB_LCK,
					memory_order_release);
				spin_unlock(lck);

				int flags = atomic_fetch_add_explicit(
					&cmsg->flags, MSG_FLAG_ANTI,
					memory_order_relaxed);
				if (flags & MSG_FLAG_PROCESSED)
					msg_queue_insert(cmsg);

				return;
			} else {
				atomic_fetch_sub_explicit(&n[i].msg_id, HB_LCK,
					memory_order_release);
			}
		}
		map_size_t tdib;
		retry_dib_check:
		tdib = (cmo + 1 + i - (msg_id_hash(td) & cmo)) & cmo;
		if (cdib > tdib) {
			if (unlikely(!atomic_compare_exchange_weak_explicit(
				&n[i].msg_id, &td, cd, memory_order_acquire,
				memory_order_relaxed))) {
				td &= ~HB_LCK;
				goto retry_dib_check;
			}
			struct lp_msg *tmsg = n[i].msg;
			simtime_t tuntil = n[i].until;
			nid_t tnid = n[i].msg_nid;
			n[i].msg = cmsg;
			n[i].until = cuntil;
			n[i].msg_nid = cnid;
			atomic_fetch_sub_explicit(&n[i].msg_id, HB_LCK,
				memory_order_release);
			cmsg = tmsg;
			cuntil = tuntil;
			cnid = tnid;
			cdib = tdib;
			cd = td | HB_LCK;
		}
		i = (i + 1) & cmo;
		++cdib;
	}
}
