/**
 * @file mm/buddy/buddy.c
 *
 * @brief A Buddy System implementation
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <mm/buddy/buddy.h>

#include <core/core.h>
#include <core/intrinsics.h>
#include <lp/lp.h>

#include <errno.h>
#include <stdlib.h>

#define left_child(i) (((i) << 1U) + 1U)
#define right_child(i) (((i) << 1U) + 2U)
#define parent(i) ((((i) + 1) >> 1U) - 1U)
#define is_power_of_2(i) (!((i) & ((i) - 1)))
#define next_exp_of_2(i) (sizeof(i) * CHAR_BIT - intrinsics_clz(i))

void model_allocator_lp_init(void)
{
	struct mm_state *self = &current_lp->mm_state;
	uint_fast8_t node_size = B_TOTAL_EXP;

	for (uint_fast32_t i = 0;
		i < sizeof(self->longest) / sizeof(*self->longest); ++i) {
		self->longest[i] = node_size;
		node_size -= is_power_of_2(i + 2);
	}

	self->used_mem = 0;
	array_init(self->logs);
}

void model_allocator_lp_fini(void)
{
	array_count_t i = array_count(current_lp->mm_state.logs);
	while (i--)
		mm_free(array_get_at(current_lp->mm_state.logs, i).c);

	array_fini(current_lp->mm_state.logs);
}

void *malloc_mt(size_t req_size)
{
	if(unlikely(!req_size))
		return NULL;

	struct mm_state *self = &current_lp->mm_state;
	uint_fast8_t req_blks = max(next_exp_of_2(req_size - 1), B_BLOCK_EXP);

	if (unlikely(self->longest[0] < req_blks)) {
		errno = ENOMEM;
		log_log(LOG_WARN, "LP %p is out of memory!", current_lp);
		return NULL;
	}

	/* search recursively for the child */
	uint_fast8_t node_size = B_TOTAL_EXP;
	uint_fast32_t i = 0;
	while (node_size > req_blks) {
		/* choose the child with smaller longest value which
		 * is still large at least *size* */
		i = left_child(i);
		i += self->longest[i] < req_blks;
		--node_size;
	}

	/* update the *longest* value back */
	self->longest[i] = 0;
	self->used_mem += 1 << node_size;

	uint_fast32_t offset = ((i + 1) << node_size) - (1 << B_TOTAL_EXP);

	while (i) {
		i = parent(i);
		self->longest[i] = max(
			self->longest[left_child(i)],
			self->longest[right_child(i)]
		);
	}

	return ((char *)self->base_mem) + offset;
}

void *calloc_mt(size_t nmemb, size_t size)
{
	size_t tot = nmemb * size;
	void *ret = malloc_mt(tot);

	if (likely(ret))
		memset(ret, 0, tot);

	return ret;
}

void free_mt(void *ptr)
{
	if (unlikely(!ptr))
		return;

	struct mm_state *self = &current_lp->mm_state;
	uint_fast8_t node_size = B_BLOCK_EXP;
	uint_fast32_t i =
		(((uintptr_t)ptr - (uintptr_t)self->base_mem) >> B_BLOCK_EXP) +
		(1 << (B_TOTAL_EXP - B_BLOCK_EXP)) - 1;

	for (; self->longest[i]; i = parent(i))
		++node_size;

	self->longest[i] = node_size;
	self->used_mem -= 1 << node_size;

	while (i) {
		i = parent(i);

		uint_fast8_t left_longest = self->longest[left_child(i)];
		uint_fast8_t right_longest = self->longest[right_child(i)];

		if (left_longest == node_size && right_longest == node_size) {
			self->longest[i] = node_size + 1;
		} else {
			self->longest[i] = max(left_longest, right_longest);
		}
		++node_size;
	}
}

void *realloc_mt(void *ptr, size_t req_size)
{
	if (!req_size) {
		free_mt(ptr);
		return NULL;
	}
	if (!ptr) {
		return malloc_mt(req_size);
	}

	abort();
	return NULL;
}

#define buddy_tree_visit(longest, on_visit)				\
__extension__({								\
	bool __vis = false;						\
	uint_fast8_t __l = B_TOTAL_EXP;					\
	uint_fast32_t __i = 0;						\
	while (1) {							\
		uint_fast8_t __lon = longest[__i];			\
		if (!__lon) {						\
			uint_fast32_t __len = 1U << __l;		\
			uint_fast32_t __o = 				\
				((__i + 1) << __l) - (1 << B_TOTAL_EXP);\
			on_visit(__o, __len);				\
		} else if (__lon != __l) {				\
			__i = left_child(__i) + __vis;			\
			__vis = false;					\
			__l--;						\
			continue;					\
		}							\
		do {							\
			__vis = !(__i & 1U);				\
			__i = parent(__i);				\
			__l++;						\
		} while (__vis);					\
									\
		if (__l > B_TOTAL_EXP) break;				\
		__vis = true;						\
	}								\
})

static struct mm_checkpoint *checkpoint_full_take(const struct mm_state *self)
{
	struct mm_checkpoint *ret = mm_alloc(
		offsetof(struct mm_checkpoint, base_mem) + self->used_mem);

	ret->used_mem = self->used_mem;
	memcpy(ret->longest, self->longest, sizeof(ret->longest));

#define buddy_block_copy_to_ckp(offset, len)				\
__extension__({								\
	memcpy(ptr, self->base_mem + offset, len);			\
	ptr += len;							\
})

	unsigned char *ptr = ret->base_mem;
	buddy_tree_visit(self->longest, buddy_block_copy_to_ckp);

#undef buddy_block_copy_to_ckp
	return ret;
}

static void checkpoint_full_restore(struct mm_state *self,
	const struct mm_checkpoint *ckp)
{
	self->used_mem = ckp->used_mem;
	memcpy(self->longest, ckp->longest, sizeof(self->longest));

#define buddy_block_copy_from_ckp(offset, len)				\
__extension__({								\
	memcpy(self->base_mem + offset, ptr, len);			\
	ptr += len;							\
})

	const unsigned char *ptr = ckp->base_mem;
	buddy_tree_visit(self->longest, buddy_block_copy_from_ckp);

#undef buddy_block_copy_from_ckp
}

void model_allocator_checkpoint_take(array_count_t ref_i)
{
	struct mm_state *self = &current_lp->mm_state;
	struct mm_log mm_log = {
		.ref_i = ref_i,
		.c = checkpoint_full_take(self)
	};
	array_push(self->logs, mm_log);
}

void model_allocator_checkpoint_next_force_full(void)
{
}

array_count_t model_allocator_checkpoint_restore(array_count_t ref_i)
{
	struct mm_state *self = &current_lp->mm_state;
	array_count_t i = array_count(self->logs) - 1;
	while (array_get_at(self->logs, i).ref_i > ref_i)
		i--;

	const struct mm_checkpoint *ckp = array_get_at(self->logs, i).c;

	checkpoint_full_restore(self, ckp);

	for (array_count_t j = array_count(self->logs) - 1; j > i; --j)
		mm_free(array_get_at(self->logs, j).c);

	array_count(self->logs) = i + 1;
	return array_get_at(self->logs, i).ref_i;
}

array_count_t model_allocator_fossil_lp_collect(struct mm_state *self,
		array_count_t tgt_ref_i)
{
	array_count_t log_i = array_count(self->logs) - 1;
	array_count_t ref_i = array_get_at(self->logs, log_i).ref_i;
	while (ref_i > tgt_ref_i) {
		--log_i;
		ref_i = array_get_at(self->logs, log_i).ref_i;
	}

	array_count_t j = array_count(self->logs);
	while (j > log_i) {
		--j;
		array_get_at(self->logs, j).ref_i -= ref_i;
	}
	while (j--)
		mm_free(array_get_at(self->logs, j).c);

	array_truncate_first(self->logs, log_i);
	return ref_i;
}
