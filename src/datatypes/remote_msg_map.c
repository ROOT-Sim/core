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
#include <lp/msg.h>
#include <mm/mm.h>

#include <memory.h>
#include <stdalign.h>

struct msg_map {
	alignas(CACHE_LINE_SIZE) atomic_uintptr_t *msgs;
	unsigned capacity;
};

static struct msg_map *re_map[MSG_ID_PHASES];
static spinlock_t *rmm_locks;
static _Thread_local spinlock_t *rmm_local_locks;

#define thread_locks_size()					\
__extension__({								\
	size_t _bt_size = n_nodes * sizeof(*rmm_locks) - 1;		\
	_bt_size = (_bt_size + CACHE_LINE_SIZE) / CACHE_LINE_SIZE;	\
	_bt_size * CACHE_LINE_SIZE;					\
})

void remote_msg_map_global_init(void)
{
	size_t m_size = n_nodes * sizeof(**re_map);

	for (unsigned i = 0; i < 3; ++i) {
		re_map[i] = mm_aligned_alloc(CACHE_LINE_SIZE, m_size);
		memset(re_map[i], 0, m_size);
	}

	re_map[MSG_ID_PHASES - 1] = re_map[2];

	rmm_locks = mm_alloc(n_threads * thread_locks_size());
	memset(rmm_locks, 0, n_threads * thread_locks_size());
}

void remote_msg_map_global_fini(void)
{
	mm_free(rmm_locks);
	for (unsigned i = 0, j = 0; i < 3; ++j) {
		if (re_map[j] == re_map[j + 1])
			continue;

		for (nid_t k = 0; k < n_nodes; ++k) {
			mm_free(re_map[j][k].msgs);
		}

		mm_free(re_map[j]);
		++i;
	}
}

void remote_msg_map_init(void)
{
	rmm_local_locks = rmm_locks + (rid * thread_locks_size());
}

inline static void remote_msg_map_lock_all(nid_t tgt_nid)
{
	const rid_t t_cnt = n_threads;
	const unsigned t_entries = thread_locks_size() / sizeof(spinlock_t);
	spinlock_t *l = rmm_locks + tgt_nid;

	for (rid_t i = 0; i < t_cnt; ++i) {
		spin_lock(l);
		l += t_entries;
	}
}

inline static void remote_msg_map_unlock_all(nid_t tgt_nid)
{
	const rid_t t_cnt = n_threads;
	const unsigned t_entries = thread_locks_size() / sizeof(spinlock_t);
	spinlock_t *l = rmm_locks + tgt_nid;

	for (rid_t i = 0; i < t_cnt; ++i) {
		spin_unlock(l);
		l += t_entries;
	}
}

void remote_msg_map_fossil_collect(unsigned new_phase)
{
	unsigned old_phase = msg_phase_previous(msg_phase_previous(new_phase));
	struct msg_map *this_map = re_map[old_phase];
	for (nid_t i = 0; i < n_nodes; ++i) {
		unsigned quota = this_map[i].capacity / n_threads;
		memset(this_map[i].msgs + quota * rid, 0,
				quota * sizeof(*this_map[i].msgs));
	}
	if (!rid) // this is not 100% safe
		re_map[msg_phase_next(new_phase)] = re_map[old_phase];
}

static void msg_map_size_increase(struct msg_map *this_map, nid_t src_nid,
		unsigned seq)
{
	remote_msg_map_lock_all(src_nid);

	if (likely(seq >= this_map->capacity)) {
		unsigned oldc = this_map->capacity;

		if (unlikely(oldc == 0))
			this_map->capacity = n_threads;

		while (seq >= this_map->capacity)
			this_map->capacity *= 2;

		this_map->msgs = mm_realloc(this_map->msgs, this_map->capacity *
				sizeof(*this_map->msgs));
		memset(this_map->msgs + oldc, 0, (this_map->capacity - oldc) *
				sizeof(*this_map->msgs));
	}

	remote_msg_map_unlock_all(src_nid);
}

void remote_msg_map_match(struct msg_id id, nid_t src_nid, struct lp_msg *msg)
{
	struct msg_map *this_map = &re_map[id.phase][src_nid];
	unsigned seq = id.seq;
	uintptr_t val = (uintptr_t) msg;
	val += msg == NULL;

	spin_lock(&rmm_local_locks[src_nid]);
	if (unlikely(seq >= this_map->capacity)) {
		spin_unlock(&rmm_local_locks[src_nid]);
		msg_map_size_increase(this_map, src_nid, seq);
		spin_lock(&rmm_local_locks[src_nid]);
	}

	val = atomic_fetch_add_explicit(&this_map->msgs[seq], val,
			memory_order_relaxed);

	spin_unlock(&rmm_local_locks[src_nid]);

	if (unlikely(val)) {
		if (likely(!msg))
			msg = (struct lp_msg *) val;

		int flags = atomic_fetch_add_explicit(&msg->flags,
				MSG_FLAG_ANTI, memory_order_relaxed);
		if (flags & MSG_FLAG_PROCESSED)
			msg_queue_insert(msg);
	}
}
