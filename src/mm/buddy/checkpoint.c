/**
 * @file mm/buddy/checkpoint.c
 *
 * @brief Buddy system checkpointing capabilities
 *
 * Provides both full and incremental checkpointing of individual buddy systems.
 * Full checkpoints save the entire allocation tree and all allocated memory blocks.
 * Incremental checkpoints save only blocks that have been dirtied since
 * the last checkpoint, as tracked by the dirty bitmap in @a buddy_state.
 * Building-block restore functions support chain-walking during incremental restore.
 *
 * SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <mm/buddy/checkpoint.h>

#include <core/core.h>


/**
 * @brief Traverses the buddy tree and performs an action on each allocated block.
 *
 * This macro iterates over the buddy tree represented by the `longest` array and
 * invokes the provided `on_visit` action for each allocated memory block.
 *
 * @param longest The array representing the buddy tree.
 * @param on_visit A callback action to perform on each allocated block. The callback
 *                 receives two parameters:
 *                 - `offset`: The offset of the block in the memory buffer.
 *                 - `length`: The size of the block.
 */
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

/**
 * @brief Computes the size in bytes required for an incremental checkpoint of this buddy system.
 *
 * The incremental checkpoint stores:
 *  - the buddy_checkpoint header fields (orig, dirty[])
 *  - one block of (1 << B_BLOCK_EXP) bytes per dirty bit in the bitmap
 *
 * @param self A pointer to the buddy system state.
 * @return The number of bytes needed.
 */
size_t buddy_checkpoint_incremental_size(const struct buddy_state *self)
{
	uint_fast32_t dirty_count = bitmap_count_set(self->dirty, sizeof(self->dirty));
	return offsetof(struct buddy_checkpoint, longest) + ((size_t)dirty_count << B_BLOCK_EXP);
}

/**
 * @brief Takes an incremental checkpoint of a buddy system.
 *
 * Saves only the dirty blocks (tracked via the dirty bitmap) into the checkpoint buffer.
 * Dirty blocks from the contiguous longest[]+base_mem[] region are packed sequentially
 * into the space normally occupied by longest[] and base_mem[].
 * After saving, the dirty bitmap is cleared.
 *
 * @param self A pointer to the buddy system state.
 * @param ret  A pointer to the destination checkpoint buffer (caller-allocated).
 * @return A pointer past the last written byte (for chaining multiple buddy checkpoints).
 */
struct buddy_checkpoint *buddy_checkpoint_incremental_take(const struct buddy_state *self, struct buddy_checkpoint *ret)
{
	ret->orig = self;
	memcpy(ret->dirty, self->dirty, sizeof(self->dirty));

	/* Pack dirty blocks sequentially into ret->longest (longest and base_mem are contiguous). */
	unsigned char *dst = ret->longest;
	const unsigned char *src = self->longest; /* longest[] and base_mem[] are contiguous */

#define incr_copy_block_to_ckp(i)                                                                                      \
	__extension__({                                                                                                \
		memcpy(dst, src + ((size_t)(i) << B_BLOCK_EXP), 1U << B_BLOCK_EXP);                                    \
		dst += 1U << B_BLOCK_EXP;                                                                              \
	})

	bitmap_foreach_set(self->dirty, sizeof(self->dirty), incr_copy_block_to_ckp);
#undef incr_copy_block_to_ckp

	// Reset dirty bitmap: blocks saved, start fresh for next checkpoint.
	buddy_dirty_reset((struct buddy_state *)self);

	return (struct buddy_checkpoint *)dst;
}

/**
 * @brief Restores dirty blocks from an incremental checkpoint (full restore, single checkpoint).
 *
 * Intended for the simple case of restoring exactly one incremental checkpoint when
 * no chain walk is needed. For chain-based restore, use
 * buddy_checkpoint_incremental_restore_partial() instead.
 *
 * @param self A pointer to the buddy system state to restore into.
 * @param ckp  A pointer to the incremental checkpoint.
 * @return A pointer past the consumed checkpoint data, or NULL if ckp doesn't match self.
 */
const struct buddy_checkpoint *buddy_checkpoint_incremental_restore(struct buddy_state *self,
    const struct buddy_checkpoint *ckp)
{
	if(ckp->orig != self)
		return NULL;

	const unsigned char *src = ckp->longest;
	unsigned char *dst = self->longest; /* longest[] and base_mem[] are contiguous */

#define incr_copy_block_from_ckp(i)                                                                                    \
	__extension__({                                                                                                \
		memcpy(dst + ((size_t)(i) << B_BLOCK_EXP), src, 1U << B_BLOCK_EXP);                                    \
		src += 1U << B_BLOCK_EXP;                                                                              \
	})

	bitmap_foreach_set(ckp->dirty, sizeof(ckp->dirty), incr_copy_block_from_ckp);
#undef incr_copy_block_from_ckp

	return (const struct buddy_checkpoint *)src;
}

/**
 * @brief Restores dirty blocks from an incremental checkpoint for blocks still in `remaining`.
 *
 * Used during backward chain traversal for incremental restore. For each dirty block in `ckp`
 * that is also set in `remaining`, the block is copied into the live buddy state and cleared
 * from `remaining`. Blocks already restored from more recent logs are skipped.
 *
 * @param self      Live buddy system to restore into.
 * @param ckp       Incremental checkpoint to restore from.
 * @param remaining Bitmap of blocks still needing restoration (modified in place).
 * @return Pointer past the consumed checkpoint data, or NULL if ckp doesn't match self.
 */
const struct buddy_checkpoint *buddy_checkpoint_incremental_restore_partial(struct buddy_state *self,
    const struct buddy_checkpoint *ckp, block_bitmap *remaining)
{
	if(ckp->orig != self)
		return NULL;

	const unsigned char *src = ckp->longest;
	unsigned char *dst = self->longest;
	uint_fast32_t block_count = bitmap_count_set(ckp->dirty, sizeof(ckp->dirty));

#define incr_partial_restore(i)                                                                                        \
	__extension__({                                                                                                \
		if(bitmap_check(remaining, (i))) {                                                                     \
			memcpy(dst + ((size_t)(i) << B_BLOCK_EXP), src, 1U << B_BLOCK_EXP);                            \
			bitmap_reset(remaining, (i));                                                                  \
		}                                                                                                      \
		src += 1U << B_BLOCK_EXP;                                                                              \
	})

	bitmap_foreach_set(ckp->dirty, sizeof(ckp->dirty), incr_partial_restore);
	(void)block_count;
#undef incr_partial_restore

	return (const struct buddy_checkpoint *)src;
}

/**
 * @brief Restores blocks still needed (`remaining`) from a full checkpoint.
 *
 * Used as the final step of backward chain traversal. Restores only the blocks
 * still set in `remaining` — blocks already restored from incremental logs are skipped.
 *
 * For the tree portion (longest[]), blocks are copied directly by index.
 * For the base_mem portion, the full checkpoint uses buddy_tree_visit to find
 * allocated blocks; only those also set in `remaining` are copied.
 *
 * @param self      Live buddy system to restore into.
 * @param ckp       Full checkpoint to restore from.
 * @param remaining Bitmap of blocks still needing restoration (read-only).
 * @return Pointer past the consumed checkpoint data, or NULL if ckp doesn't match self.
 */
const struct buddy_checkpoint *buddy_checkpoint_full_restore_remaining(struct buddy_state *self,
    const struct buddy_checkpoint *ckp, const block_bitmap *remaining)
{
	if(ckp->orig != self)
		return NULL;

	const unsigned int tree_blocks = (1U << (B_TOTAL_EXP - 2 * B_BLOCK_EXP + 1));

	// Restore tree portion: copy only blocks set in remaining
	for(unsigned int i = 0; i < tree_blocks; i++) {
		if(bitmap_check(remaining, i)) {
			memcpy(self->longest + ((size_t)i << B_BLOCK_EXP), ckp->longest + ((size_t)i << B_BLOCK_EXP),
			    1U << B_BLOCK_EXP);
		}
	}

	// Restore base_mem portion via tree visit (full ckp stores only allocated blocks)
	const unsigned char *ptr = ckp->base_mem;

#define full_restore_remaining_block(offset, len)                                                                      \
	__extension__({                                                                                                \
		uint_fast32_t _off = (offset);                                                                         \
		uint_fast32_t _len = (len);                                                                            \
		do {                                                                                                   \
			uint_fast32_t _bi = (_off >> B_BLOCK_EXP) + tree_blocks;                                       \
			if(bitmap_check(remaining, _bi))                                                               \
				memcpy(self->base_mem + _off, ptr, 1U << B_BLOCK_EXP);                                 \
			ptr += 1U << B_BLOCK_EXP;                                                                      \
			_off += 1U << B_BLOCK_EXP;                                                                     \
			_len -= 1U << B_BLOCK_EXP;                                                                     \
		} while(_len);                                                                                         \
	})

	buddy_tree_visit(ckp->longest, full_restore_remaining_block);
#undef full_restore_remaining_block

	return (const struct buddy_checkpoint *)ptr;
}

/**
 * @brief Takes a full checkpoint.
 *
 * This function creates a full checkpoint of the given buddy system state by copying
 * the current state of the allocation tree and memory buffer into the provided checkpoint structure.
 *
 * @param self A pointer to the `buddy_state` structure representing the current buddy system state.
 * @param ret A pointer to the `buddy_checkpoint` structure where the checkpoint will be stored.
 * @return A pointer to the next available memory location after the checkpoint data.
 */
struct buddy_checkpoint *buddy_checkpoint_full_take(const struct buddy_state *self, struct buddy_checkpoint *ret)
{
	ret->orig = self;
	if(global_config.incremental_ckpt)
		memcpy(ret->dirty, self->dirty, sizeof(self->dirty));
	memcpy(ret->longest, self->longest, sizeof(ret->longest));

#define buddy_block_copy_to_ckp(offset, len)                                                                           \
	__extension__({                                                                                                \
		memcpy(ptr, self->base_mem + (offset), (len));                                                         \
		ptr += (len);                                                                                          \
	})

	unsigned char *ptr = ret->base_mem;
	buddy_tree_visit(self->longest, buddy_block_copy_to_ckp);

#undef buddy_block_copy_to_ckp
	return (struct buddy_checkpoint *)ptr;
}

/**
 * @brief Restores the full state.
 *
 * This function restores the state of the buddy system, including the allocation tree
 * and memory buffer, from the provided checkpoint. It ensures that the checkpoint
 * corresponds to the given buddy system before performing the restoration.
 *
 * @param self A pointer to the `buddy_state` structure representing the current buddy system.
 * @param ckpt A pointer to the `buddy_checkpoint` structure containing the checkpoint data.
 * @return A pointer to the next available memory location after the checkpoint data,
 *         or `NULL` if the checkpoint does not match the buddy system.
 */
const struct buddy_checkpoint *buddy_checkpoint_full_restore(struct buddy_state *self,
    const struct buddy_checkpoint *ckpt)
{
	if(unlikely(ckpt->orig != self))
		return NULL;

	memcpy(self->longest, ckpt->longest, sizeof(self->longest));

#define buddy_block_copy_from_ckp(offset, len)                                                                         \
	__extension__({                                                                                                \
		memcpy(self->base_mem + (offset), ptr, (len));                                                         \
		ptr += (len);                                                                                          \
	})

	const unsigned char *ptr = ckpt->base_mem;
	buddy_tree_visit(self->longest, buddy_block_copy_from_ckp);

#undef buddy_block_copy_from_ckp
	return (const struct buddy_checkpoint *)ptr;
}
