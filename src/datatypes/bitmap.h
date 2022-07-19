/**
 * @file datatypes/bitmap.h
 *
 * @brief Bitmap data type
 *
 * This a simple bitmap implemented with some simple macros. Keep in mind that
 * some trust is given to the developer since the implementation, for
 * performances and simplicity reasons, doesn't remember its effective size;
 * consequently it doesn't check boundaries on the array that stores the bits.
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <core/intrinsics.h>

#include <limits.h> // for CHAR_BIT
#include <memory.h> // for memset()

/// The type of a generic bitmap.
typedef unsigned char block_bitmap;

/* macros for internal use */

/// The primitive type used to build a bitmap
#define B_BLOCK_TYPE uint_fast32_t
/// The size of the primitive type to build a bitmap
#define B_BLOCK_SIZE ((unsigned)sizeof(B_BLOCK_TYPE))
/// Number of bits in a primitive type to build a bitmap
#define B_BITS_PER_BLOCK (B_BLOCK_SIZE * CHAR_BIT)
/// Mask used to fiddle single bits
#define B_MASK ((B_BLOCK_TYPE)1U)
/// Union cast to access the bitmap
#define B_UNION_CAST(bitmap) ((B_BLOCK_TYPE *)(bitmap))

/// B_BITS_PER_BLOCK is a power of 2 in any real architecture
#define B_MOD_OF_BPB(n) (((unsigned)(n)) & ((unsigned)(B_BITS_PER_BLOCK - 1)))

/// Macro to set a bit in a primitive block composing a bitmap
#define B_SET_BIT_AT(B, K) ((B) |= (B_MASK << (K)))
/// Macro to clear a bit in a primitive block composing a bitmap
#define B_RESET_BIT_AT(B, K) ((B) &= ~(B_MASK << (K)))
/// Macro to check if a bit is set in a primitive block composing a bitmap
#define B_CHECK_BIT_AT(B, K) ((B) & (B_MASK << (K)))

/// Macro to set a bit in a bitmap
#define B_SET_BIT(A, I) B_SET_BIT_AT((A)[((I) / B_BITS_PER_BLOCK)], (B_MOD_OF_BPB(I)))
/// Macro to clear a bit in a bitmap
#define B_RESET_BIT(A, I) B_RESET_BIT_AT((A)[((I) / B_BITS_PER_BLOCK)], (B_MOD_OF_BPB(I)))
/// Macro to check if a bit is set in a bitmap
#define B_CHECK_BIT(A, I) B_CHECK_BIT_AT((A)[((I) / B_BITS_PER_BLOCK)], (B_MOD_OF_BPB(I)))

/**
 * @brief Computes the required size of a bitmap
 * @param requested_bits the requested number of bits.
 * @returns the size in bytes of the requested bitmap.
 *
 * For example this statically declares a 100 entries bitmap and initializes it:
 * 		block_bitmap my_bitmap[bitmap_required_size(100)] = {0};
 * Avoid side effects in the arguments, they may be evaluated more than once.
 */
#define bitmap_required_size(requested_bits)                                                                           \
	((((requested_bits) / B_BITS_PER_BLOCK) + (B_MOD_OF_BPB(requested_bits) != 0)) * B_BLOCK_SIZE)

/**
 * @brief Initializes a bitmap
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
#define bitmap_initialize(memory_pointer, requested_bits)                                                              \
	memset(memory_pointer, 0, bitmap_required_size(requested_bits))

/**
 * @brief Sets a bit in a bitmap
 * @param bitmap a pointer to the bitmap to write.
 * @param bit_index the index of the bit to set.
 *
 * Avoid side effects in the arguments, they may be evaluated more than once.
 */
#define bitmap_set(bitmap, bit_index) (B_SET_BIT(B_UNION_CAST(bitmap), ((unsigned)(bit_index))))

/**
 * @brief Resets a bit in a bitmap
 * @param bitmap a pointer to the bitmap to write.
 * @param bit_index the index of the bit to reset.
 *
 * Avoid side effects in the arguments, they may be evaluated more than once.
 */
#define bitmap_reset(bitmap, bit_index) (B_RESET_BIT(B_UNION_CAST(bitmap), ((unsigned)(bit_index))))

/**
 * @brief Checks a bit in a bitmap
 * @param bitmap a pointer to the bitmap.
 * @param bit_index the index of the bit to read
 * @return 0 if the bit is not set, 1 otherwise
 *
 * Avoid side effects in the arguments, they may be evaluated more than once.
 */
#define bitmap_check(bitmap, bit_index) (B_CHECK_BIT(B_UNION_CAST(bitmap), ((unsigned)(bit_index))) != 0)

/**
 * @brief Counts the occurrences of set bits in a bitmap
 * @param bitmap a pointer to the bitmap.
 * @param bitmap_size the size of the bitmap in bytes
 * @return the number of cleared bits in the bitmap
 *
 * This macro expects the number of bits in the bitmap to be a multiple of B_BITS_PER_BLOCK.
 * Avoid side effects in the arguments, they may be evaluated more than once.
 */
#define bitmap_count_set(bitmap, bitmap_size)                                                                          \
	__extension__({                                                                                                \
		unsigned __i = (bitmap_size) / B_BLOCK_SIZE;                                                           \
		unsigned __ret = 0;                                                                                    \
		B_BLOCK_TYPE *__block_b = B_UNION_CAST(bitmap);                                                        \
		while(__i--) {                                                                                         \
			__ret += intrinsics_popcount(__block_b[__i]);                                                  \
		}                                                                                                      \
		__ret;                                                                                                 \
	})

/**
 * @brief Counts the occurrences of cleared bits in a bitmap
 * @param bitmap a pointer to the bitmap.
 * @param bitmap_size the size of the bitmap in bytes
 * @return the number of cleared bits in the bitmap
 *
 * This macro expects the number of bits in the bitmap to be a multiple of B_BITS_PER_BLOCK.
 * Avoid side effects in the arguments, they may be evaluated more than once.
 */
#define bitmap_count_reset(bitmap, bitmap_size)                                                                        \
	__extension__({ (bitmap_size) * CHAR_BIT - bitmap_count_set(bitmap, bitmap_size); })

/**
 * @brief Computes the index of the first cleared bit in a bitmap.
 * @param bitmap a pointer to the bitmap.
 * @param bitmap_size the size of the bitmap in bytes
 * @return the index of the first cleared bit in the bitmap, UINT_MAX if none is found.
 *
 * This macro expects the number of bits in the bitmap to be a multiple of B_BITS_PER_BLOCK.
 * Avoid side effects in the arguments, they may be evaluated more than once.
 */
#define bitmap_first_reset(bitmap, bitmap_size)                                                                        \
	__extension__({                                                                                                \
		unsigned __i, __blocks = (bitmap_size) / B_BLOCK_SIZE;                                                 \
		unsigned __ret = UINT_MAX;                                                                             \
		B_BLOCK_TYPE __cur_block, *__block_b = B_UNION_CAST(bitmap);                                           \
		for(__i = 0; __i < __blocks; ++__i) {                                                                  \
			if((__cur_block = ~__block_b[__i])) {                                                          \
				__ret = intrinsics_ctz(__cur_block);                                                   \
				break;                                                                                 \
			}                                                                                              \
		}                                                                                                      \
		__ret;                                                                                                 \
	})

/**
 * @brief Executes a user supplied function for each set bit in a bitmap.
 * @param bitmap a pointer to the bitmap.
 * @param bitmap_size the size of the bitmap in bytes
 * @param func a function which takes a single unsigned argument, the index of the current set bit.
 *
 * This macro expects the number of bits in the bitmap to be a multiple of B_BITS_PER_BLOCK.
 * Avoid side effects in the arguments, they may be evaluated more than once.
 */
#define bitmap_foreach_set(bitmap, bitmap_size, func)                                                                  \
	__extension__({                                                                                                \
		unsigned __i, __fnd, __blocks = (bitmap_size) / B_BLOCK_SIZE;                                          \
		B_BLOCK_TYPE __cur_block, *__block_b = B_UNION_CAST(bitmap);                                           \
		for(__i = 0; __i < __blocks; ++__i) {                                                                  \
			if((__cur_block = __block_b[__i])) {                                                           \
				do {                                                                                   \
					__fnd = intrinsics_ctz(__cur_block);                                           \
					B_RESET_BIT_AT(__cur_block, __fnd);                                            \
					func((__fnd + __i * B_BITS_PER_BLOCK));                                        \
				} while(__cur_block);                                                                  \
			}                                                                                              \
		}                                                                                                      \
	})

/**
 * @brief Merges a bitmap into another one by OR-ing all the bits.
 * @param dest a pointer to the destination bitmap.
 * @param source a pointer to the source bitmap.
 * @param bitmap_size the size of the bitmap in bytes
 * @return the index of the first cleared bit in the bitmap, UINT_MAX if none is found.
 *
 * This macro expects the number of bits in the bitmap to be a multiple of B_BITS_PER_BLOCK.
 * Avoid side effects in the arguments, they may be evaluated more than once.
 */
#define bitmap_merge_or(dest, source, bitmap_size)                                                                     \
	__extension__({                                                                                                \
		unsigned __i = (bitmap_size) / B_BLOCK_SIZE;                                                           \
		B_BLOCK_TYPE *__s_blocks = B_UNION_CAST(source);                                                       \
		B_BLOCK_TYPE *__d_blocks = B_UNION_CAST(dest);                                                         \
		while(__i--) {                                                                                         \
			__d_blocks[__i] |= __s_blocks[__i];                                                            \
		}                                                                                                      \
		__d_blocks;                                                                                            \
	})
