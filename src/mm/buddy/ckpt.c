/**
* @file mm/buddy/ckpt.c
*
* @brief Checkpointing capabilities
*
* SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
* SPDX-License-Identifier: GPL-3.0-only
*/
#include <mm/buddy/ckpt.h>

#include <core/core.h>
#include <datatypes/array.h>
#include <mm/mm.h>

#define MAX_WASTED_SPACE 0.2

static __thread double used_mem_avg;
static __thread dyn_array(struct mm_checkpoint *) avail_ckpts;

void model_allocator_init(void)
{
	array_init(avail_ckpts);
}

void model_allocator_fini(void)
{
	while(array_count(avail_ckpts))
		mm_free(array_pop(avail_ckpts));
	array_fini(avail_ckpts);
}

static struct mm_checkpoint *ckpt_alloc(uint_fast32_t used_mem)
{
	used_mem_avg = 0.95 * used_mem_avg + 0.05 * (1.0 + MAX_WASTED_SPACE) * (double)used_mem;

	struct mm_checkpoint *ret;
	if(unlikely(used_mem > used_mem_avg)) {
		ret = mm_alloc(offsetof(struct mm_checkpoint, base_mem) + used_mem);
		ret->ckpt_size = used_mem;
		return ret;
	}

	while(likely(array_count(avail_ckpts))) {
		ret = array_pop(avail_ckpts);
		if(likely(ret->used_mem >= used_mem))
			return ret;
		mm_free(ret);
	}

	ret = mm_alloc(offsetof(struct mm_checkpoint, base_mem) + used_mem_avg);
	ret->ckpt_size = (uint_fast32_t) used_mem_avg;

	return ret;
}

void checkpoint_full_free(struct mm_checkpoint *ckpt)
{
	if(unlikely(ckpt->ckpt_size < used_mem_avg)) {
		mm_free(ckpt);
		return;
	}
	array_push(avail_ckpts, ckpt);
}


#define buddy_tree_visit(longest, on_visit)                                                                            \
	__extension__({                                                                                                \
		bool __vis = false;                                                                                    \
		uint_fast8_t __l = B_TOTAL_EXP;                                                                        \
		uint_fast32_t __i = 0;                                                                                 \
		while(1) {                                                                                             \
			uint_fast8_t __lon = (longest)[__i];                                                             \
			if(!__lon) {                                                                                   \
				uint_fast32_t __len = 1U << __l;                                                       \
				uint_fast32_t __o = ((__i + 1) << __l) - (1 << B_TOTAL_EXP);                           \
				on_visit(__o, __len);                                                                  \
			} else if(__lon != __l) {                                                                      \
				__i = buddy_left_child(__i) + __vis;                                                   \
				__vis = false;                                                                         \
				__l--;                                                                                 \
				continue;                                                                              \
			}                                                                                              \
			do {                                                                                           \
				__vis = !(__i & 1U);                                                                   \
				__i = buddy_parent(__i);                                                               \
				__l++;                                                                                 \
			} while(__vis);                                                                                \
                                                                                                                       \
			if(__l > B_TOTAL_EXP)                                                                          \
				break;                                                                                 \
			__vis = true;                                                                                  \
		}                                                                                                      \
	})

struct mm_checkpoint *checkpoint_full_take(const struct mm_state *self)
{
	struct mm_checkpoint *ret = ckpt_alloc(self->used_mem);

	ret->used_mem = self->used_mem;
	memcpy(ret->longest, self->longest, sizeof(ret->longest));

#define buddy_block_copy_to_ckp(offset, len)                                                                           \
	__extension__({                                                                                                \
		memcpy(ptr, self->base_mem + offset, len);                                                             \
		ptr += len;                                                                                            \
	})

	unsigned char *ptr = ret->base_mem;
	buddy_tree_visit(self->longest, buddy_block_copy_to_ckp);

#undef buddy_block_copy_to_ckp
	return ret;
}

void checkpoint_full_restore(struct mm_state *self, const struct mm_checkpoint *ckp)
{
	self->used_mem = ckp->used_mem;
	memcpy(self->longest, ckp->longest, sizeof(self->longest));

#define buddy_block_copy_from_ckp(offset, len)                                                                         \
	__extension__({                                                                                                \
		memcpy(self->base_mem + offset, ptr, len);                                                             \
		ptr += len;                                                                                            \
	})

	const unsigned char *ptr = ckp->base_mem;
	buddy_tree_visit(self->longest, buddy_block_copy_from_ckp);

#undef buddy_block_copy_from_ckp
}
