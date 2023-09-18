/**
 * @file mm/buddy/ckpt.c
 *
 * @brief Checkpointing capabilities
 *
 * SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <mm/buddy/ckpt.h>

#include <core/core.h>


#define buddy_tree_visit(longest, on_visit)                                                                            \
	__extension__({                                                                                                \
		bool __vis = false;                                                                                    \
		uint_fast8_t __l = B_TOTAL_EXP;                                                                        \
		uint_fast32_t __i = 0;                                                                                 \
		while(1) {                                                                                             \
			uint_fast8_t __lon = (longest)[__i];                                                           \
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

// TODO: fix incremental checkpointing
#ifdef ROOTSIM_INCREMENTAL

struct buddy_checkpoint *checkpoint_incremental_take(const struct buddy_state *self)
{
	uint_fast32_t bset = bitmap_count_set(self->dirty, sizeof(self->dirty));

	struct buddy_checkpoint *ret = mm_alloc(offsetof(struct buddy_checkpoint, longest) + bset * (1 << B_BLOCK_EXP));

	unsigned char *ptr = ret->longest;
	const unsigned char *src = self->longest;

#define copy_block_to_ckp(i)                                                                                           \
	__extension__({                                                                                                \
		memcpy(ptr, src + ((i) << B_BLOCK_EXP), 1 << B_BLOCK_EXP);                                             \
		ptr += 1 << B_BLOCK_EXP;                                                                               \
	})

	bitmap_foreach_set(self->dirty, sizeof(self->dirty), copy_block_to_ckp);
#undef copy_block_to_ckp

	memcpy(ret->dirty, self->dirty, sizeof(self->dirty));
	return ret;
}

void checkpoint_incremental_restore(struct buddy_state *self, const struct buddy_checkpoint *ckp)
{
	array_count_t i = array_count(self->logs) - 1;
	const struct buddy_checkpoint *cur_ckp = array_get_at(self->logs, i).c;

	while(cur_ckp != ckp) {
		bitmap_merge_or(self->dirty, cur_ckp->dirty, sizeof(self->dirty));
		cur_ckp = array_get_at(self->logs, --i).c;
	}

#define copy_dirty_block(i)                                                                                            \
	__extension__({                                                                                                \
		if(bitmap_check(self->dirty, i)) {                                                                     \
			memcpy(self->longest + (i << B_BLOCK_EXP), ptr, 1 << B_BLOCK_EXP);                             \
			bitmap_reset(self->dirty, i);                                                                  \
			--r;                                                                                           \
		}                                                                                                      \
		ptr += 1 << B_BLOCK_EXP;                                                                               \
	})

#define copy_block_from_ckp(i)                                                                                         \
	__extension__({                                                                                                \
		memcpy(self->longest + (i << B_BLOCK_EXP), cur_ckp->longest + (i << B_BLOCK_EXP), 1 << B_BLOCK_EXP);   \
		--r;                                                                                                   \
	})

#define buddy_block_dirty_from_ckp(offset, len)                                                                        \
	__extension__({                                                                                                \
		uint_fast32_t i = (offset >> B_BLOCK_EXP) + (1 << (B_TOTAL_EXP - 2 * B_BLOCK_EXP + 1));                \
		uint_fast32_t b_len = len;                                                                             \
		do {                                                                                                   \
			copy_dirty_block(i);                                                                           \
			i++;                                                                                           \
			b_len -= 1U << B_BLOCK_EXP;                                                                    \
		} while(b_len);                                                                                        \
	})

	uint_fast32_t r = bitmap_count_set(self->dirty, sizeof(self->dirty));
	const unsigned char *ptr = cur_ckp->longest;

	bitmap_foreach_set(cur_ckp->dirty, sizeof(cur_ckp->dirty), copy_dirty_block);

	const unsigned tree_bit_size = bitmap_required_size(1 << (B_TOTAL_EXP - 2 * B_BLOCK_EXP + 1));

	while(r) {
		cur_ckp = array_get_at(self->logs, --i).c;
		if(cur_ckp->is_incremental) {
			ptr = cur_ckp->longest;
			bitmap_foreach_set(cur_ckp->dirty, sizeof(cur_ckp->dirty), copy_dirty_block);
		} else {
			bitmap_foreach_set(self->dirty, tree_bit_size, copy_block_from_ckp);
			ptr = cur_ckp->base_mem;
			buddy_tree_visit(cur_ckp->longest, buddy_block_dirty_from_ckp);
		}
	}

#undef copy_dirty_block
#undef copy_block_from_ckp
#undef buddy_block_dirty_from_ckp
}

#endif

struct buddy_checkpoint *checkpoint_full_take(const struct buddy_state *self, struct buddy_checkpoint *ret)
{
	ret->orig = self->chunk;
#ifdef ROOTSIM_INCREMENTAL
	memcpy(ret->dirty, self->dirty, sizeof(self->dirty));
#endif
	memcpy(ret->longest, self->longest, sizeof(ret->longest));

#define buddy_block_copy_to_ckp(offset, len)                                                                           \
	__extension__({                                                                                                \
		memcpy(ptr, self->chunk->mem + (offset), (len));                                                       \
		ptr += (len);                                                                                          \
	})

	unsigned char *ptr = ret->base_mem;
	buddy_tree_visit(self->longest, buddy_block_copy_to_ckp);

#undef buddy_block_copy_to_ckp
	return (struct buddy_checkpoint *)ptr;
}

const struct buddy_checkpoint *checkpoint_full_restore(struct buddy_state *self, const struct buddy_checkpoint *ckp)
{
	if(unlikely(ckp->orig != self->chunk))
		return NULL;

	memcpy(self->longest, ckp->longest, sizeof(self->longest));

#define buddy_block_copy_from_ckp(offset, len)                                                                         \
	__extension__({                                                                                                \
		memcpy(self->chunk->mem + (offset), ptr, (len));                                                       \
		ptr += (len);                                                                                          \
	})

	const unsigned char *ptr = ckp->base_mem;
	buddy_tree_visit(self->longest, buddy_block_copy_from_ckp);

#undef buddy_block_copy_from_ckp
	return (const struct buddy_checkpoint *)ptr;
}
