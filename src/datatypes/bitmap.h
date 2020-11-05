/**
* @file datatypes/bitmap.h
*
* @brief Bitmap data type
*
* This a simple bitmap implemented with some simple macros.
* Keep in mind that some trust is given to the developer since
* the implementation, for performances and simplicity
* reasons, doesn't remember its effective size; consequently
* it doesn't check boundaries on the array that stores the bits.
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
#pragma once

#include <core/intrinsics.h>

#include <limits.h>		// for CHAR_BIT
#include <memory.h>		// for memset()

/// This defines a generic bitmap.
typedef unsigned char block_bitmap;

/* macros for internal use */
#define B_BLOCK_TYPE uint_fast32_t
#define B_BLOCK_SIZE ((unsigned) sizeof(B_BLOCK_TYPE))
#define B_BITS_PER_BLOCK (B_BLOCK_SIZE * CHAR_BIT)
#define B_MASK ((B_BLOCK_TYPE)1U)
#define B_UNION_CAST(bitmap) ((B_BLOCK_TYPE*)(bitmap))

// B_BITS_PER_BLOCK is a power of 2 in any real architecture
#define B_MOD_OF_BPB(n) (((unsigned)(n)) & ((unsigned)(B_BITS_PER_BLOCK - 1)))

#define B_SET_BIT_AT(B,K) 	( B |= (B_MASK << K) )
#define B_RESET_BIT_AT(B,K) 	( B &= ~(B_MASK << K) )
#define B_CHECK_BIT_AT(B,K) 	( B & (B_MASK << K) )

#define B_SET_BIT(A, I) 						\
	B_SET_BIT_AT((A)[((I) / B_BITS_PER_BLOCK)], (B_MOD_OF_BPB(I)))

#define B_RESET_BIT(A, I) 						\
	B_RESET_BIT_AT((A)[((I) / B_BITS_PER_BLOCK)], (B_MOD_OF_BPB(I)))

#define B_CHECK_BIT(A, I) 						\
	B_CHECK_BIT_AT((A)[((I) / B_BITS_PER_BLOCK)], (B_MOD_OF_BPB(I)))

/**
 * @brief Computes the required size of a bitmap with @a requested_bits entries.
 * @param requested_bits the requested number of bits.
 * @returns the size in bytes of the requested bitmap.
 *
 * For example this statically declares a 100 entries bitmap and initializes it:
 * 		block_bitmap my_bitmap[bitmap_required_size(100)] = {0};
 * Avoid side effects in the arguments, they may be evaluated more than once.
 */
#define bitmap_required_size(requested_bits)				\
	((								\
		((requested_bits) / B_BITS_PER_BLOCK) + 		\
		(B_MOD_OF_BPB(requested_bits) != 0)			\
	) * B_BLOCK_SIZE)

/**
 * @brief Initializes the bitmap at @a memory_pointer containing @a requested_bits entries.
 * @param memory_pointer the pointer to the bitmap to initialize.
 * @param requested_bits the number of bits contained in the bitmap.
 * @returns the very same @a memory_pointer
 *
 * The argument @a requested_bits is necessary since the bitmap is "dumb"
 * For example this dynamically declares a 100 entries bitmap and initializes it:
 * 		block_bitmap *my_bitmap = malloc(bitmap_required_size(100));
 * 		bitmap_initialize(my_bitmap, 100);
 * Avoid side effects in the arguments, they may be evaluated more than once.
 */
#define bitmap_initialize(memory_pointer, requested_bits)		\
	memset(memory_pointer, 0, bitmap_required_size(requested_bits))

/**
 * @brief This sets the bit with index @a bit_index of the bitmap @a bitmap
 * @param bitmap a pointer to the bitmap to write.
 * @param bit_index the index of the bit to set.
 *
 * Avoid side effects in the arguments, they may be evaluated more than once.
 */
#define bitmap_set(bitmap, bit_index)					\
	(B_SET_BIT(B_UNION_CAST(bitmap), ((unsigned)(bit_index))))

/**
 * @brief This resets the bit with index @a bit_index of the bitmap @a bitmap
 * @param bitmap a pointer to the bitmap to write.
 * @param bit_index the index of the bit to reset.
 *
 * Avoid side effects in the arguments, they may be evaluated more than once.
 */
#define bitmap_reset(bitmap, bit_index)					\
	(B_RESET_BIT(B_UNION_CAST(bitmap), ((unsigned)(bit_index))))

/**
 * @brief This checks if the bit with index @a bit_index of the bitmap @a bitmap is set or unset.
 * @param bitmap a pointer to the bitmap.
 * @param bit_index the index of the bit to read
 * @return 0 if the bit is not set, 1 otherwise
 *
 * Avoid side effects in the arguments, they may be evaluated more than once.
 */
#define bitmap_check(bitmap, bit_index)					\
	(B_CHECK_BIT(B_UNION_CAST(bitmap), ((unsigned)(bit_index))) != 0)

/**
 * @brief This counts the occurrences of set bits in the bitmap @a bitmap.
 * @param bitmap a pointer to the bitmap.
 * @param bitmap_size the size of the bitmap in bytes (obtainable through bitmap_required_size())
 * @return the number of cleared bits in the bitmap
 *
 * This macro expects the number of bits in the bitmap to be a multiple of B_BITS_PER_BLOCK.
 * Avoid side effects in the arguments, they may be evaluated more than once.
 */
#define bitmap_count_set(bitmap, bitmap_size) 				\
__extension__({ 							\
	unsigned __i = bitmap_size / B_BLOCK_SIZE;			\
	unsigned __ret = 0;						\
	B_BLOCK_TYPE *__block_b = B_UNION_CAST(bitmap);			\
	while (__i--){							\
		__ret += SAFE_POPC(__block_b[__i]);			\
	}								\
	__ret; 								\
})

/**
 * @brief This counts the occurrences of cleared bits in the bitmap @a bitmap.
 * @param bitmap a pointer to the bitmap.
 * @param bitmap_size the size of the bitmap in bytes (obtainable through bitmap_required_size())
 * @return the number of cleared bits in the bitmap
 *
 * This macro expects the number of bits in the bitmap to be a multiple of B_BITS_PER_BLOCK.
 * Avoid side effects in the arguments, they may be evaluated more than once.
 */
#define bitmap_count_reset(bitmap, bitmap_size)				\
__extension__({								\
	bitmap_size * CHAR_BIT - bitmap_count_set(bitmap, bitmap_size); \
})

/**
 * @brief This returns the index of the first cleared bit in @a bitmap.
 * @param bitmap a pointer to the bitmap.
 * @param bitmap_size the size of the bitmap in bytes (obtainable through bitmap_required_size())
 * @return the index of the first cleared bit in the bitmap, UINT_MAX if none is found.
 *
 * This macro expects the number of bits in the bitmap to be a multiple of B_BITS_PER_BLOCK.
 * Avoid side effects in the arguments, they may be evaluated more than once.
 */
#define bitmap_first_reset(bitmap, bitmap_size)				\
__extension__({								\
	unsigned __i, __blocks = bitmap_size / B_BLOCK_SIZE;		\
	unsigned __ret = UINT_MAX;					\
	B_BLOCK_TYPE __cur_block,					\
		*__block_b = B_UNION_CAST(bitmap);			\
	for(__i = 0; __i < __blocks; ++__i){				\
		if((__cur_block = ~__block_b[__i])){			\
			__ret = B_CTZ(__cur_block);			\
			break;						\
		}							\
	}								\
	__ret; 								\
})

/**
 * @brief This executes a user supplied function for each set bit in @a bitmap.
 * @param bitmap a pointer to the bitmap.
 * @param bitmap_size the size of the bitmap in bytes (obtainable through bitmap_required_size())
 * @param func a function which takes a single unsigned argument, the index of the current set bit.
 *
 * This macro expects the number of bits in the bitmap to be a multiple of B_BITS_PER_BLOCK.
 * Avoid side effects in the arguments, they may be evaluated more than once.
 */
#define bitmap_foreach_set(bitmap, bitmap_size, func) 			\
__extension__({ 							\
	unsigned __i, __fnd, __blocks = bitmap_size / B_BLOCK_SIZE;	\
	B_BLOCK_TYPE __cur_block, *__block_b = B_UNION_CAST(bitmap);	\
	for(__i = 0; __i < __blocks; ++__i){				\
		if((__cur_block = __block_b[__i])){			\
			do {						\
				__fnd = SAFE_CTZ(__cur_block);		\
				B_RESET_BIT_AT(__cur_block, __fnd);	\
				func((__fnd + __i * B_BITS_PER_BLOCK));	\
			} while(__cur_block);				\
		}							\
	}								\
})

/**
 * @brief This merges the bitmap @a source into the @a dest bitmap by OR-ing all the bits.
 * @param dest a pointer to the destination bitmap.
 * @param source a pointer to the source bitmap.
 * @param bitmap_size the size of the bitmap in bytes (obtainable through bitmap_required_size())
 * @return the index of the first cleared bit in the bitmap, UINT_MAX if none is found.
 *
 * This macro expects the number of bits in the bitmap to be a multiple of B_BITS_PER_BLOCK.
 * Avoid side effects in the arguments, they may be evaluated more than once.
 */
#define bitmap_merge_or(dest, source, bitmap_size)			\
__extension__({ 							\
	unsigned __i = bitmap_size / B_BLOCK_SIZE;			\
	B_BLOCK_TYPE *__s_blocks = B_UNION_CAST(source);		\
	B_BLOCK_TYPE *__d_blocks = B_UNION_CAST(dest);			\
	while (__i--){							\
		__d_blocks[__i] |= __s_blocks[__i];			\
	}								\
	__d_blocks; 							\
})
