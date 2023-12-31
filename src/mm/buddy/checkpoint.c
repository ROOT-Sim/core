/**
 * @file mm/buddy/ckpt.c
 *
 * @brief Checkpointing capabilities
 *
 * SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <mm/buddy/checkpoint.h>

#include <core/core.h>

#include <memory.h>

#define buddy_tree_visit(longest, on_visit)                                                                            \
	__extension__({                                                                                                \
		bool __vis = false;                                                                                    \
		uint_fast8_t __l = B_TOTAL_EXP;                                                                        \
		uint_fast32_t __i = 1;                                                                                 \
		while(1) {                                                                                             \
			uint_fast8_t __lon = (longest)[__i >> 1U];                                                     \
                        __lon = (__i & 1U) ? __lon & 0x0f : __lon >> 4;                                                \
			if(!__lon) {                                                                                   \
				uint_fast32_t __len = 1U << __l;                                                       \
				uint_fast32_t __o = (__i << __l) - (1 << B_TOTAL_EXP);                                 \
				on_visit(__o, __len);                                                                  \
			} else if(__lon + B_BLOCK_EXP - 1 != __l) {                                                    \
				__i = (__i << 1U) + __vis;                                                             \
				__vis = false;                                                                         \
				__l--;                                                                                 \
				continue;                                                                              \
			}                                                                                              \
			do {                                                                                           \
				__vis = __i & 1U;                                                                      \
				__i >>= 1U;                                                                            \
				__l++;                                                                                 \
			} while(__vis);                                                                                \
                                                                                                                       \
			if(__l > B_TOTAL_EXP)                                                                          \
				break;                                                                                 \
			__vis = true;                                                                                  \
		}                                                                                                      \
	})

struct buddy_checkpoint *checkpoint_full_take(const struct buddy_state *self, struct buddy_checkpoint *ret)
{
	ret->buddy = *self;

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
	memcpy(self->longest, ckp->buddy.longest, sizeof(self->longest));

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
