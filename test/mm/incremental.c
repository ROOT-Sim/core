/**
 * @file test/mm/incremental.c
 *
 * @brief Test: Incremental checkpointing at buddy-system and WriteMemory level
 *
 * Tests cover two groups:
 *   Group A: WriteMemory dirty-bitmap marking — verifies that the compiler-injected
 *            instrumentation hook correctly marks dirty blocks in the bitmap.
 *   Group B: Buddy-level incremental checkpoint take/restore — verifies the full
 *            checkpoint chain: full → incremental → incremental → restore.
 *
 * SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <test.h>
#include <mock.h>

#include <lp/lp.h>
#include <mm/buddy/buddy.h>
#include <mm/buddy/checkpoint.h>
#include <mm/checkpoint/checkpoint.h>
#include <mm/model_allocator.h>

#include <string.h>
#include <stdint.h>

/* -------------------------------------------------------------------------
 * Group A: WriteMemory dirty-bitmap marking tests
 * ---------------------------------------------------------------------- */

/**
 * Test that WriteMemory is a no-op when incremental_ckpt is disabled.
 */
static int test_write_mem_disabled(void *_)
{
	(void)_;
	struct lp_ctx *lp = test_lp_mock_get();
	current_lp = lp;
	model_allocator_lp_init(&lp->mm_state);

	/* Incremental disabled: WriteMemory must be a no-op. */
	global_config.incremental_ckpt = false;

	unsigned char *buf = rs_malloc(128);
	test_assert(buf != NULL);

	/* Record state of dirty bitmaps before. */
	struct buddy_state *buddy = buddy_find_by_address(&lp->mm_state, buf);
	test_assert(buddy != NULL);

	/* Clear dirty bits (they may have been set by malloc in incremental mode). */
	buddy_dirty_reset(buddy);

	/* Call WriteMemory: should NOT set any dirty bits. */
	WriteMemory(buf, 128);

	int errs = 0;
	for(size_t i = 0; i < sizeof(buddy->dirty); i++)
		errs += buddy->dirty[i] != 0;

	rs_free(buf);
	model_allocator_lp_fini(&lp->mm_state);
	global_config.incremental_ckpt = false;
	return errs > 0;
}

/**
 * Test that WriteMemory correctly marks dirty bits when enabled.
 * Writes to a known address and verifies the corresponding bitmap bits are set.
 */
static int test_write_mem_marks_dirty(void *_)
{
	(void)_;
	struct lp_ctx *lp = test_lp_mock_get();
	current_lp = lp;
	model_allocator_lp_init(&lp->mm_state);
	global_config.incremental_ckpt = true;

	/* Allocate a 64-byte block (one bitmap block). */
	unsigned char *buf = rs_malloc(1U << B_BLOCK_EXP);
	test_assert(buf != NULL);

	struct buddy_state *buddy = buddy_find_by_address(&lp->mm_state, buf);
	test_assert(buddy != NULL);

	/* Reset dirty bitmaps. */
	buddy_dirty_reset(buddy);

	/* Mark as dirty via WriteMemory. */
	WriteMemory(buf, 1U << B_BLOCK_EXP);

	/* At least one dirty bit in the base_mem region must be set. */
	int errs = 1;
	const uint32_t tree_blocks = (1U << (B_TOTAL_EXP - 2 * B_BLOCK_EXP + 1));
	for(uint32_t i = tree_blocks; i < tree_blocks + (1U << (B_TOTAL_EXP - B_BLOCK_EXP)); i++) {
		if(bitmap_check(buddy->dirty, i)) {
			errs = 0;
			break;
		}
	}

	rs_free(buf);
	model_allocator_lp_fini(&lp->mm_state);
	global_config.incremental_ckpt = false;
	return errs;
}

/**
 * Test that WriteMemory is a no-op for pointers outside the buddy range.
 */
static int test_write_mem_out_of_range(void *_)
{
	(void)_;
	struct lp_ctx *lp = test_lp_mock_get();
	current_lp = lp;
	model_allocator_lp_init(&lp->mm_state);
	global_config.incremental_ckpt = true;

	unsigned char *buf = rs_malloc(64);
	test_assert(buf != NULL);

	struct buddy_state *buddy = buddy_find_by_address(&lp->mm_state, buf);
	test_assert(buddy != NULL);
	buddy_dirty_reset(buddy);

	/* Pointer before the first buddy: should be ignored. */
	WriteMemory((char *)array_get_at(lp->mm_state.buddies, 0) - 1, 64);

	int errs = 0;
	for(size_t i = 0; i < sizeof(buddy->dirty); i++)
		errs += buddy->dirty[i] != 0;

	rs_free(buf);
	model_allocator_lp_fini(&lp->mm_state);
	global_config.incremental_ckpt = false;
	return errs > 0;
}

/**
 * Test that buddy_malloc sets dirty bits in the tree region when incremental is enabled.
 */
static int test_alloc_marks_tree_dirty(void *_)
{
	(void)_;
	struct lp_ctx *lp = test_lp_mock_get();
	current_lp = lp;
	model_allocator_lp_init(&lp->mm_state);
	global_config.incremental_ckpt = true;

	/* Must allocate first so the buddy system is created. */
	void *buf = rs_malloc(64);
	test_assert(buf != NULL);

	/* Get the buddy that owns buf and reset dirty bits. */
	struct buddy_state *buddy = buddy_find_by_address(&lp->mm_state, buf);
	test_assert(buddy != NULL);
	buddy_dirty_reset(buddy);

	/* Free and re-alloc: buddy_malloc and buddy_free must dirty tree bits. */
	rs_free(buf);
	buf = rs_malloc(64);
	test_assert(buf != NULL);
	buddy = buddy_find_by_address(&lp->mm_state, buf);

	int found = 0;
	const uint32_t tree_blocks = (1U << (B_TOTAL_EXP - 2 * B_BLOCK_EXP + 1));
	for(uint32_t i = 0; i < tree_blocks; i++) {
		if(bitmap_check(buddy->dirty, i)) {
			found = 1;
			break;
		}
	}

	rs_free(buf);
	model_allocator_lp_fini(&lp->mm_state);
	global_config.incremental_ckpt = false;
	return !found;
}

/* -------------------------------------------------------------------------
 * Group B: Buddy-level incremental checkpoint take/restore
 * ---------------------------------------------------------------------- */

#define INCR_TEST_PATTERN_A 0xAAAAAAAAAAAAAAAAULL
#define INCR_TEST_PATTERN_B 0xBBBBBBBBBBBBBBBBULL
#define INCR_TEST_PATTERN_C 0xCCCCCCCCCCCCCCCCULL

/**
 * Test incremental take/restore for a single buddy, single level.
 * Sequence: alloc → full checkpoint → modify → incremental checkpoint →
 *           corrupt → restore incremental → verify.
 */
static int test_incremental_single(void *_)
{
	(void)_;
	struct lp_ctx *lp = test_lp_mock_get();
	current_lp = lp;
	model_allocator_lp_init(&lp->mm_state);
	global_config.incremental_ckpt = true;

	/* Allocate two 64-byte blocks and fill with pattern A. */
	uint64_t *a = rs_malloc(64);
	uint64_t *b = rs_malloc(64);
	test_assert(a && b);
	memset(a, 0xAA, 64);
	memset(b, 0xAA, 64);

	/* Force full checkpoint at index 0. */
	model_allocator_checkpoint_next_force_full(&lp->mm_state);
	model_allocator_checkpoint_take(&lp->mm_state, 0);

	/* Modify b with pattern B, mark dirty via WriteMemory. */
	WriteMemory(b, 64);
	memset(b, 0xBB, 64);

	/* Take incremental checkpoint at index 1. */
	model_allocator_checkpoint_take(&lp->mm_state, 1);

	/* Corrupt both blocks. */
	memset(a, 0xFF, 64);
	memset(b, 0xFF, 64);

	/* Restore to incremental checkpoint (index 1).
	 * b should be restored to pattern B; a was not dirty so stays FF
	 * (it will be restored from full when chain walk reaches it). */
	model_allocator_checkpoint_restore(&lp->mm_state, 1);

	/* After restore to checkpoint 1: b = BB (was dirty in incr), a = AA (from full or not modified) */
	int errs = 0;
	for(int i = 0; i < 8; i++) {
		errs += b[i] != INCR_TEST_PATTERN_B;
		errs += a[i] != INCR_TEST_PATTERN_A;
	}

	rs_free(a);
	rs_free(b);
	model_allocator_lp_fini(&lp->mm_state);
	global_config.incremental_ckpt = false;
	return errs > 0;
}

/**
 * Test a chain: full checkpoint → incr 1 → incr 2 → restore to full.
 * Verifies that both incremental logs are properly skipped during fossil
 * collection and the chain walk reaches the full checkpoint.
 */
static int test_incremental_chain(void *_)
{
	(void)_;
	struct lp_ctx *lp = test_lp_mock_get();
	current_lp = lp;
	model_allocator_lp_init(&lp->mm_state);
	global_config.incremental_ckpt = true;

	uint64_t *a = rs_malloc(64);
	uint64_t *b = rs_malloc(64);
	test_assert(a && b);

	/* Initial state: pattern A. */
	memset(a, 0xAA, 64);
	memset(b, 0xAA, 64);

	/* Full checkpoint at index 0. */
	model_allocator_checkpoint_next_force_full(&lp->mm_state);
	model_allocator_checkpoint_take(&lp->mm_state, 0);

	/* Modify a → incremental checkpoint at index 1. */
	WriteMemory(a, 64);
	memset(a, 0xBB, 64);
	model_allocator_checkpoint_take(&lp->mm_state, 1);

	/* Modify b → incremental checkpoint at index 2. */
	WriteMemory(b, 64);
	memset(b, 0xCC, 64);
	model_allocator_checkpoint_take(&lp->mm_state, 2);

	/* Corrupt everything. */
	memset(a, 0xFF, 64);
	memset(b, 0xFF, 64);

	/* Restore to checkpoint 0 (full): both blocks should be AA. */
	model_allocator_checkpoint_restore(&lp->mm_state, 0);

	int errs = 0;
	for(int i = 0; i < 8; i++) {
		errs += a[i] != INCR_TEST_PATTERN_A;
		errs += b[i] != INCR_TEST_PATTERN_A;
	}

	rs_free(a);
	rs_free(b);
	model_allocator_lp_fini(&lp->mm_state);
	global_config.incremental_ckpt = false;
	return errs > 0;
}

/**
 * Test that full checkpointing still works correctly when incremental mode
 * is enabled (full checkpoint resets dirty bitmaps).
 */
static int test_full_resets_dirty(void *_)
{
	(void)_;
	struct lp_ctx *lp = test_lp_mock_get();
	current_lp = lp;
	model_allocator_lp_init(&lp->mm_state);
	global_config.incremental_ckpt = true;

	void *buf = rs_malloc(64);
	test_assert(buf != NULL);

	struct buddy_state *buddy = buddy_find_by_address(&lp->mm_state, buf);

	/* Some dirty bits should be set from malloc. */
	int any_dirty_before = 0;
	for(size_t i = 0; i < sizeof(buddy->dirty); i++)
		any_dirty_before |= buddy->dirty[i];

	/* Full checkpoint must reset dirty bitmap. */
	model_allocator_checkpoint_next_force_full(&lp->mm_state);
	model_allocator_checkpoint_take(&lp->mm_state, 0);

	int any_dirty_after = 0;
	for(size_t i = 0; i < sizeof(buddy->dirty); i++)
		any_dirty_after |= buddy->dirty[i];

	rs_free(buf);
	model_allocator_lp_fini(&lp->mm_state);
	global_config.incremental_ckpt = false;

	/* Must have had dirty bits before, none after the full checkpoint. */
	return !any_dirty_before || any_dirty_after;
}

/* -------------------------------------------------------------------------
 * Test entry point
 * ---------------------------------------------------------------------- */

int incremental_checkpoint_test(_unused void *_)
{
	int errs = 0;

	/* Group A: WriteMemory */
	errs += test_write_mem_disabled(NULL);
	errs += test_write_mem_marks_dirty(NULL);
	errs += test_write_mem_out_of_range(NULL);
	errs += test_alloc_marks_tree_dirty(NULL);

	/* Group B: incremental checkpoint chain */
	errs += test_incremental_single(NULL);
	errs += test_incremental_chain(NULL);
	errs += test_full_resets_dirty(NULL);

	return errs > 0;
}
