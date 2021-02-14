/**
 * @file datatypes/remote_msg_map.c
 *
 * @brief Message map datatype
 *
 * Message map datatype
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <datatypes/remote_msg_map.h>

#include <core/intrinsics.h>
#include <core/sync.h>
#include <datatypes/msg_queue.h>
#include <gvt/gvt.h>
#include <lp/msg.h>
#include <mm/mm.h>

#include <memory.h>
#include <stdalign.h>

#define MAX_LOAD_FACTOR 0.95
#define HB_LCK ((uintptr_t)1U)

typedef uint_fast32_t map_size_t;

/// A bucket of the remote messages hash map
struct msg_map_node {
	/// The id of the registered message
	atomic_uintptr_t msg_id;
	/// The node id of the sender of the registered message
	nid_t msg_nid;
	/// The logical time after which it is safe to delete this entry
	simtime_t until;
	/// The registered message, NULL if it is an anti-message entry
	struct lp_msg *msg;
};

/// The message hash map used to register external messages
struct msg_map {
	alignas(CACHE_LINE_SIZE) struct {
		/// The array of hash map nodes
		struct msg_map_node *nodes;
		/// The current capacity of this hashmap minus one
		/** A capacity in the form 2^n - 1 eases modulo calculations */
		map_size_t capacity_mo;
		/// The count of entries which can still be inserted
		/** Takes into account load factor, when 0 we have to resize */
		atomic_int count;
	};
	struct {
		/// Synchronizes access from the worker threads
		alignas(CACHE_LINE_SIZE) spinlock_t l;
	} locks[MAX_THREADS];
};

static struct msg_map re_map;

static __attribute__((const)) inline map_size_t msg_id_hash(uintptr_t msg_id)
{
	return (msg_id ^ (msg_id >> 32)) * 0xbf58476d1ce4e5b9ULL; // @suppress("Avoid magic numbers")
}

void remote_msg_map_global_init(void)
{
	map_size_t cnt = n_threads * 2 / (1 - MAX_LOAD_FACTOR);
	map_size_t cap = 1ULL << (sizeof(cnt) * CHAR_BIT - intrinsics_clz(cnt));
	// capacity_mo is in the form 2^n - 1, modulo computations are then easy
	re_map.capacity_mo = cap - 1;
	atomic_store_explicit(&re_map.count, cap * MAX_LOAD_FACTOR,
			      memory_order_relaxed);
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
	static struct msg_map_node *old_nodes = NULL;
	static atomic_int re_map_bar = 0;
	static map_size_t cap;

	if (!atomic_fetch_add_explicit(&re_map_bar, 1U, memory_order_acquire)) {
		remote_msg_map_lock_all();
		mm_free(old_nodes);
		old_nodes = re_map.nodes;
		cap = re_map.capacity_mo + 1;
		atomic_fetch_sub_explicit(&re_map_bar, n_threads,
			memory_order_release);

		re_map.nodes = mm_alloc(sizeof(struct msg_map_node) * cap);
		memset(re_map.nodes, 0, sizeof(struct msg_map_node) * cap);
		atomic_store_explicit(&re_map.count, cap * MAX_LOAD_FACTOR,
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
		struct msg_map_node *node = &old_nodes[off + cnt];
		if (node->msg_id && current_gvt <= node->until)
			remote_msg_map_match(node->msg_id, node->msg_nid,
				node->msg);
	}
}

static void msg_map_size_increase(void)
{
	const map_size_t old_cmo = re_map.capacity_mo;
	const map_size_t cmo = old_cmo * 2 + 1;
	struct msg_map_node *old_nodes = re_map.nodes;
	struct msg_map_node *nodes = mm_alloc(sizeof(*nodes) * (cmo + 1));

	memset(nodes, 0, sizeof(*nodes) * (cmo + 1));

	remote_msg_map_lock_all();

	atomic_fetch_add_explicit(&re_map.count, old_cmo * MAX_LOAD_FACTOR,
		memory_order_release);

	for (map_size_t j = 0; j <= old_cmo; ++j) {
		if(!old_nodes[j].msg_id)
			continue;

		struct msg_map_node *cnode = &old_nodes[j];
		map_size_t cdib = 0;
		map_size_t i = msg_id_hash(atomic_load_explicit(&cnode->msg_id,
			memory_order_relaxed)) & cmo;

		while (nodes[i].msg_id) {
			map_size_t tdib = (cmo + 1 + i - (msg_id_hash(
				atomic_load_explicit(&nodes[i].msg_id,
				memory_order_relaxed)) & cmo)) & cmo;

			if (cdib > tdib) {
				cdib = tdib;
				struct msg_map_node tmp_node;
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

	struct msg_map_node *n = re_map.nodes;
	const map_size_t cmo = re_map.capacity_mo;
	i &= cmo;
	// linear probing with robin hood hashing
	// https://cs.uwaterloo.ca/research/tr/1986/CS-86-14.pdf by Pedro Celis
	while (1) {
		uintptr_t td = atomic_load_explicit(&n[i].msg_id,
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
