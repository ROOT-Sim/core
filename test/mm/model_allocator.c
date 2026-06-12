/**
 * @file test/mm/model_allocator.c
 *
 * @brief Test: model allocator: rs_malloc/calloc/free/realloc + checkpointing
 *
 * Covers all functionalities of the model allocator in both full and
 * incremental checkpointing modes. The tests are organized in groups:
 *
 * Group A: lp_init field initialisation
 * Group B: rs_malloc / rs_free / buddy_find_by_address / full_ckpt_size
 * Group C: rs_calloc (zero-fill + dirty mark)
 * Group D: rs_realloc (NULL-ptr, zero-size, in-place, copy path, dirty mark)
 * Group E: full checkpointing: ckpt_size / incr_ckpt_size fields, restore
 * Group F: incremental checkpointing: take / restore mid-chain / incr_ckpt_size
 * Group G: model_allocator_fossil_lp_collect: ref_idx rebase, chain anchor
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

#include <errno.h>
#include <stdint.h>

extern void __write_mem(const void *ptr, size_t size);
#define BUDDY_CAPACITY (1U << B_TOTAL_EXP)
#define BLOCK_SIZE (1U << B_BLOCK_EXP)

static struct lp_ctx *setup_lp(bool incremental)
{
	struct lp_ctx *lp = test_lp_mock_get();
	current_lp = lp;
	model_allocator_lp_init(&lp->mm_state);
	global_config.incremental_ckpt = incremental;
	global_config.full_ckpt_period = 0;
	return lp;
}

static void teardown_lp(struct lp_ctx *lp)
{
	model_allocator_lp_fini(&lp->mm_state);
	global_config.incremental_ckpt = false;
	global_config.full_ckpt_period = 0;
}

/* ======================================
 * Group A: lp_init field initialisation
 * ====================================== */

/**
 * After model_allocator_lp_init, all mm_state fields must be in a well-defined
 * initial state: empty arrays, correct full_ckpt_size baseline, and zero/NULL
 * for the incremental fields.
 */
static int test_lp_init_fields(void *_)
{
	(void)_;
	struct lp_ctx *lp = setup_lp(false);
	struct mm_state *s = &lp->mm_state;

	test_assert(array_count(s->buddies) == 0);
	test_assert(array_count(s->logs) == 0);

	/*
	 * full_ckpt_size baseline = header + sentinel pointer:
	 * offsetof(mm_checkpoint, chkps) + sizeof(buddy_state *)
	 */
	const uint_fast32_t expected_base = offsetof(struct mm_checkpoint, chkps) + sizeof(struct buddy_state *);
	test_assert(s->full_ckpt_size == expected_base);

	test_assert(s->force_full == false);
	test_assert(s->ckpt_since_last_full == 0);
	test_assert(s->last_dirty_buddy == NULL);

	teardown_lp(lp);
	return 0;
}

/* ======================================================================
 * Group B: rs_malloc / rs_free / buddy_find_by_address / full_ckpt_size
 * ====================================================================== */

/**
 * rs_malloc(0) must return NULL without touching mm_state.
 */
static int test_malloc_zero(void *_)
{
	(void)_;
	struct lp_ctx *lp = setup_lp(false);

	uint_fast32_t size_before = lp->mm_state.full_ckpt_size;
	void *p = rs_malloc(0);
	test_assert(p == NULL);
	test_assert(lp->mm_state.full_ckpt_size == size_before);
	test_assert(array_count(lp->mm_state.buddies) == 0);

	teardown_lp(lp);
	return 0;
}

/**
 * A small allocation must:
 * - return a non-NULL pointer
 * - create exactly one buddy system
 * - increase full_ckpt_size by BLOCK_SIZE + buddy-header overhead
 * After free, full_ckpt_size must drop by BLOCK_SIZE.
 */
static int test_malloc_single_block(void *_)
{
	(void)_;
	struct lp_ctx *lp = setup_lp(false);

	const uint_fast32_t base = lp->mm_state.full_ckpt_size;
	void *p = rs_malloc(1);
	test_assert(p != NULL);
	test_assert(array_count(lp->mm_state.buddies) == 1);

	const uint_fast32_t buddy_hdr = offsetof(struct buddy_checkpoint, base_mem);
	test_assert(lp->mm_state.full_ckpt_size == base + BLOCK_SIZE + buddy_hdr);

	rs_free(p);
	test_assert(lp->mm_state.full_ckpt_size == base + buddy_hdr);

	teardown_lp(lp);
	return 0;
}

/** rs_free(NULL) must be a safe no-op. */
static int test_free_null(void *_)
{
	(void)_;
	struct lp_ctx *lp = setup_lp(false);

	void *p = rs_malloc(BLOCK_SIZE);
	test_assert(p != NULL);
	uint_fast32_t size_with = lp->mm_state.full_ckpt_size;

	rs_free(NULL);
	test_assert(lp->mm_state.full_ckpt_size == size_with);

	rs_free(p);
	teardown_lp(lp);
	return 0;
}

/**
 * buddy_find_by_address must return the buddy that owns a given pointer,
 * and the pointer must lie in that buddy's base_mem range.
 */
static int test_find_by_address(void *_)
{
	(void)_;
	struct lp_ctx *lp = setup_lp(false);

	void *p = rs_malloc(BLOCK_SIZE * 4);
	test_assert(p != NULL);

	struct buddy_state *buddy = buddy_find_by_address(&lp->mm_state, p);
	test_assert(buddy != NULL);
	test_assert((unsigned char *)p >= buddy->base_mem);
	test_assert((unsigned char *)p < buddy->base_mem + BUDDY_CAPACITY);

	rs_free(p);
	teardown_lp(lp);
	return 0;
}

/**
 * When the first buddy is full a second must be allocated and sorted into
 * the array.
 * buddy_find_by_address must correctly resolve the pointer in the second buddy.
 */
static int test_malloc_second_buddy(void *_)
{
	(void)_;
	struct lp_ctx *lp = setup_lp(false);

	void *fill = rs_malloc(BUDDY_CAPACITY);
	test_assert(fill != NULL);
	test_assert(array_count(lp->mm_state.buddies) == 1);

	void *p = rs_malloc(BLOCK_SIZE);
	test_assert(p != NULL);
	test_assert(array_count(lp->mm_state.buddies) == 2);

	test_assert(array_get_at(lp->mm_state.buddies, 0) < array_get_at(lp->mm_state.buddies, 1));

	struct buddy_state *b = buddy_find_by_address(&lp->mm_state, p);
	test_assert(b != NULL);
	test_assert((unsigned char *)p >= b->base_mem);
	test_assert((unsigned char *)p < b->base_mem + BUDDY_CAPACITY);

	rs_free(fill);
	rs_free(p);
	teardown_lp(lp);
	return 0;
}

/**
 * full_ckpt_size must be consistent: the full checkpoint's ckpt_size and
 * incr_ckpt_size must both equal full_ckpt_size after a full checkpoint.
 */
static int test_full_ckpt_size_consistency(void *_)
{
	(void)_;
	struct lp_ctx *lp = setup_lp(false);

	void *ptrs[8];
	const size_t sizes[8] = {64, 128, 64, 256, 64, 128, 64, 128};
	for(int i = 0; i < 8; i++)
		ptrs[i] = rs_malloc(sizes[i]);

	rs_free(ptrs[2]);
	ptrs[2] = NULL;
	rs_free(ptrs[5]);
	ptrs[5] = NULL;

	model_allocator_checkpoint_next_force_full(&lp->mm_state);
	model_allocator_checkpoint_take(&lp->mm_state, 0);

	test_assert(array_count(lp->mm_state.logs) == 1);
	struct mm_log entry = array_get_at(lp->mm_state.logs, 0);
	test_assert(!is_log_incremental(entry));

	struct mm_checkpoint *ckpt = log_get_ckpt(entry);
	test_assert(ckpt->ckpt_size == lp->mm_state.full_ckpt_size);
	test_assert(ckpt->incr_ckpt_size == lp->mm_state.full_ckpt_size);

	for(int i = 0; i < 8; i++)
		if(ptrs[i])
			rs_free(ptrs[i]);
	teardown_lp(lp);
	return 0;
}

/* ===================
 * Group C: rs_calloc
 * =================== */

/** rs_calloc must return a zero-filled region. */
static int test_calloc_zeroed(void *_)
{
	(void)_;
	struct lp_ctx *lp = setup_lp(false);

	const size_t n = 16, sz = 8;
	unsigned char *p = rs_calloc(n, sz);
	test_assert(p != NULL);

	int errs = 0;
	for(size_t i = 0; i < n * sz; i++)
		errs += p[i] != 0;

	rs_free(p);
	teardown_lp(lp);
	return errs > 0;
}

/**
 * In incremental mode, rs_calloc must mark the data region dirty so the
 * zero-fill is captured in the next incremental checkpoint.
 */
static int test_calloc_marks_dirty(void *_)
{
	(void)_;
	struct lp_ctx *lp = setup_lp(true);

	unsigned char *p = rs_calloc(1, BLOCK_SIZE);
	test_assert(p != NULL);

	struct buddy_state *buddy = buddy_find_by_address(&lp->mm_state, p);
	test_assert(buddy != NULL);

	const uint32_t tree_blocks = (1U << (B_TOTAL_EXP - 2 * B_BLOCK_EXP + 1));
	int found = 0;
	for(uint32_t i = tree_blocks; i < tree_blocks + (1U << (B_TOTAL_EXP - B_BLOCK_EXP)); i++) {
		if(bitmap_check(buddy->dirty, i)) {
			found = 1;
			break;
		}
	}

	rs_free(p);
	teardown_lp(lp);
	return !found;
}

/* =====================
 * Group D: rs_realloc
 * ===================== */

/** rs_realloc(NULL, size) must behave like rs_malloc(size). */
static int test_realloc_null_ptr(void *_)
{
	(void)_;
	struct lp_ctx *lp = setup_lp(false);

	void *p = rs_realloc(NULL, BLOCK_SIZE * 2);
	test_assert(p != NULL);
	test_assert(array_count(lp->mm_state.buddies) == 1);

	rs_free(p);
	teardown_lp(lp);
	return 0;
}

/** rs_realloc(NULL, 0) and rs_realloc(ptr, 0) must return NULL + EINVAL. */
static int test_realloc_zero_size(void *_)
{
	(void)_;
	struct lp_ctx *lp = setup_lp(false);

	errno = 0;
	void *r = rs_realloc(NULL, 0);
	test_assert(r == NULL);
	test_assert(errno == EINVAL);

	void *p = rs_malloc(BLOCK_SIZE);
	test_assert(p != NULL);
	errno = 0;
	r = rs_realloc(p, 0);
	test_assert(r == NULL);

	rs_free(p);
	teardown_lp(lp);
	return 0;
}

/**
 * rs_realloc to the same rounded block size must be in-place:
 * the pointer must be unchanged, and data must be preserved.
 */
static int test_realloc_same_size_inplace(void *_)
{
	(void)_;
	struct lp_ctx *lp = setup_lp(false);

	void *p = rs_malloc(BLOCK_SIZE);
	test_assert(p != NULL);
	memset(p, 0xAB, BLOCK_SIZE);

	void *q = rs_realloc(p, BLOCK_SIZE);
	test_assert(q == p);

	int errs = 0;
	for(size_t i = 0; i < BLOCK_SIZE; i++)
		errs += ((unsigned char *)q)[i] != 0xAB;

	rs_free(q);
	teardown_lp(lp);
	return errs > 0;
}

/**
 * rs_realloc to a larger size (different rounded block size) triggers the
 * alloc-copy-free path. In incremental mode, the destination must be dirtied.
 */
static int test_realloc_copy_path(void *_)
{
	(void)_;
	struct lp_ctx *lp = setup_lp(true);

	unsigned char *p = rs_malloc(BLOCK_SIZE);
	test_assert(p != NULL);
	memset(p, 0xCD, BLOCK_SIZE);
	__write_mem(p, BLOCK_SIZE);

	const size_t new_size = BLOCK_SIZE * 4;
	unsigned char *q = rs_realloc(p, new_size);
	test_assert(q != NULL);

	int errs = 0;
	for(size_t i = 0; i < BLOCK_SIZE; i++)
		errs += q[i] != 0xCD;

	struct buddy_state *buddy = buddy_find_by_address(&lp->mm_state, q);
	test_assert(buddy != NULL);
	const uint32_t tree_blocks = (1U << (B_TOTAL_EXP - 2 * B_BLOCK_EXP + 1));
	int dirty_found = 0;
	for(uint32_t i = tree_blocks; i < tree_blocks + (1U << (B_TOTAL_EXP - B_BLOCK_EXP)); i++) {
		if(bitmap_check(buddy->dirty, i)) {
			dirty_found = 1;
			break;
		}
	}
	errs += !dirty_found;

	rs_free(q);
	teardown_lp(lp);
	return errs > 0;
}

/* =============================
 * Group E: Full checkpointing
 * ============================= */

/**
 * A full checkpoint + restore must reproduce the exact memory contents
 * present at checkpoint time.
 */
static int test_full_checkpoint_restore(void *_)
{
	(void)_;
	struct lp_ctx *lp = setup_lp(false);

	uint64_t *buf = rs_malloc(sizeof(uint64_t) * 8);
	test_assert(buf != NULL);
	for(uint64_t i = 0; i < 8; i++)
		buf[i] = i * 0x1111111111111111ULL;

	model_allocator_checkpoint_next_force_full(&lp->mm_state);
	model_allocator_checkpoint_take(&lp->mm_state, 0);

	test_assert(array_count(lp->mm_state.logs) == 1);
	test_assert(!is_log_incremental(array_get_at(lp->mm_state.logs, 0)));

	struct mm_checkpoint *ckpt = log_get_ckpt(array_get_at(lp->mm_state.logs, 0));
	test_assert(ckpt->ckpt_size == lp->mm_state.full_ckpt_size);
	test_assert(ckpt->incr_ckpt_size == lp->mm_state.full_ckpt_size);

	memset(buf, 0xFF, sizeof(uint64_t) * 8);
	model_allocator_checkpoint_restore(&lp->mm_state, 0);

	int errs = 0;
	for(uint64_t i = 0; i < 8; i++)
		errs += buf[i] != i * 0x1111111111111111ULL;

	rs_free(buf);
	teardown_lp(lp);
	return errs > 0;
}

/**
 * Multiple full checkpoints: restore to an intermediate one must reproduce
 * that exact state, and later logs must be freed.
 */
static int test_full_multiple_checkpoints(void *_)
{
	(void)_;
	struct lp_ctx *lp = setup_lp(false);

	uint64_t *buf = rs_malloc(sizeof(uint64_t) * 4);
	test_assert(buf != NULL);

	memset(buf, 0xAA, sizeof(uint64_t) * 4);
	model_allocator_checkpoint_next_force_full(&lp->mm_state);
	model_allocator_checkpoint_take(&lp->mm_state, 10);

	memset(buf, 0xBB, sizeof(uint64_t) * 4);
	model_allocator_checkpoint_next_force_full(&lp->mm_state);
	model_allocator_checkpoint_take(&lp->mm_state, 20);

	memset(buf, 0xCC, sizeof(uint64_t) * 4);
	model_allocator_checkpoint_next_force_full(&lp->mm_state);
	model_allocator_checkpoint_take(&lp->mm_state, 30);

	test_assert(array_count(lp->mm_state.logs) == 3);

	// Restore to ref 20 (buf = BBBB)
	array_count_t restored = model_allocator_checkpoint_restore(&lp->mm_state, 20);
	test_assert(restored == 20);

	int errs = 0;
	for(int i = 0; i < 4; i++)
		errs += buf[i] != 0xBBBBBBBBBBBBBBBBULL;

	// Only checkpoints 0..1 must remain
	test_assert(array_count(lp->mm_state.logs) == 2);

	// full_ckpt_size after restore must match the restored checkpoint
	test_assert(lp->mm_state.full_ckpt_size == log_get_ckpt(array_get_at(lp->mm_state.logs, 1))->ckpt_size);

	rs_free(buf);
	teardown_lp(lp);
	return errs > 0;
}

/**
 * force_full must be cleared and ckpt_since_last_full reset to 0 after
 * a full checkpoint is taken.
 */
static int test_force_full_flag_reset(void *_)
{
	(void)_;
	struct lp_ctx *lp = setup_lp(true);

	void *p = rs_malloc(BLOCK_SIZE);
	test_assert(p != NULL);

	model_allocator_checkpoint_next_force_full(&lp->mm_state);
	test_assert(lp->mm_state.force_full == true);

	model_allocator_checkpoint_take(&lp->mm_state, 0);
	test_assert(lp->mm_state.force_full == false);
	test_assert(lp->mm_state.ckpt_since_last_full == 0);

	rs_free(p);
	teardown_lp(lp);
	return 0;
}

/* ====================================
 * Group F: Incremental checkpointing
 * ==================================== */

/**
 * An incremental checkpoint must have incr_ckpt_size < ckpt_size
 * (only dirty blocks are saved, not the whole state).
 */
static int test_incremental_smaller_than_full(void *_)
{
	(void)_;
	struct lp_ctx *lp = setup_lp(true);

	uint64_t *buf = rs_malloc(sizeof(uint64_t) * 8);
	test_assert(buf != NULL);
	memset(buf, 0xAA, sizeof(uint64_t) * 8);

	model_allocator_checkpoint_next_force_full(&lp->mm_state);
	model_allocator_checkpoint_take(&lp->mm_state, 0);

	// Dirty only one word
	__write_mem(&buf[0], sizeof(uint64_t));
	buf[0] = 0xDEADBEEFCAFEBABEULL;
	model_allocator_checkpoint_take(&lp->mm_state, 1);

	test_assert(array_count(lp->mm_state.logs) == 2);
	struct mm_log incr_log = array_get_at(lp->mm_state.logs, 1);
	test_assert(is_log_incremental(incr_log));

	struct mm_checkpoint *incr_ckpt = log_get_ckpt(incr_log);
	test_assert(incr_ckpt->ckpt_size == lp->mm_state.full_ckpt_size);
	test_assert(incr_ckpt->incr_ckpt_size < incr_ckpt->ckpt_size);

	rs_free(buf);
	teardown_lp(lp);
	return 0;
}

/**
 * With full_ckpt_period = 3, the 3rd incremental must be promoted to a full
 * checkpoint, resetting ckpt_since_last_full to 0.
 */
static int test_full_ckpt_period(void *_)
{
	(void)_;
	struct lp_ctx *lp = setup_lp(true);
	global_config.full_ckpt_period = 3;

	void *p = rs_malloc(BLOCK_SIZE);
	test_assert(p != NULL);

	model_allocator_checkpoint_next_force_full(&lp->mm_state);
	model_allocator_checkpoint_take(&lp->mm_state, 0);
	test_assert(lp->mm_state.ckpt_since_last_full == 0);

	for(int i = 1; i <= 3; i++) {
		__write_mem(p, BLOCK_SIZE);
		model_allocator_checkpoint_take(&lp->mm_state, (array_count_t)i);
	}

	test_assert(lp->mm_state.ckpt_since_last_full == 0);
	test_assert(!is_log_incremental(array_get_at(lp->mm_state.logs, array_count(lp->mm_state.logs) - 1)));

	rs_free(p);
	teardown_lp(lp);
	return 0;
}

/**
 * Restore to an intermediate incremental in a chain (full→incr1→incr2):
 * restoring to incr1 must produce the state at incr1.
 */
static int test_incremental_restore_mid_chain(void *_)
{
	(void)_;
	struct lp_ctx *lp = setup_lp(true);

	uint64_t *a = rs_malloc(sizeof(uint64_t) * 8);
	uint64_t *b = rs_malloc(sizeof(uint64_t) * 8);
	test_assert(a && b);

	memset(a, 0xAA, sizeof(uint64_t) * 8);
	memset(b, 0xAA, sizeof(uint64_t) * 8);

	model_allocator_checkpoint_next_force_full(&lp->mm_state);
	model_allocator_checkpoint_take(&lp->mm_state, 0);

	__write_mem(a, sizeof(uint64_t) * 8);
	memset(a, 0xBB, sizeof(uint64_t) * 8);
	model_allocator_checkpoint_take(&lp->mm_state, 1);

	__write_mem(b, sizeof(uint64_t) * 8);
	memset(b, 0xCC, sizeof(uint64_t) * 8);
	model_allocator_checkpoint_take(&lp->mm_state, 2);

	memset(a, 0xFF, sizeof(uint64_t) * 8);
	memset(b, 0xFF, sizeof(uint64_t) * 8);

	// Restore to incr1: a = BB, b = AA
	array_count_t r = model_allocator_checkpoint_restore(&lp->mm_state, 1);
	test_assert(r == 1);

	int errs = 0;
	for(int i = 0; i < 8; i++) {
		errs += a[i] != 0xBBBBBBBBBBBBBBBBULL;
		errs += b[i] != 0xAAAAAAAAAAAAAAAAULL;
	}

	test_assert(lp->mm_state.full_ckpt_size == log_get_ckpt(array_get_at(lp->mm_state.logs, 1))->ckpt_size);

	// Dirty bitmaps must be clear after restore
	for(array_count_t i = 0; i < array_count(lp->mm_state.buddies); i++) {
		struct buddy_state *buddy = array_get_at(lp->mm_state.buddies, i);
		for(size_t j = 0; j < sizeof(buddy->dirty); j++)
			errs += buddy->dirty[j] != 0;
	}

	rs_free(a);
	rs_free(b);
	teardown_lp(lp);
	return errs > 0;
}

/**
 * Restore to the full baseline from the tail of an incremental chain
 * must reproduce the state at the full checkpoint.
 */
static int test_incremental_restore_to_full(void *_)
{
	(void)_;
	struct lp_ctx *lp = setup_lp(true);

	uint64_t *buf = rs_malloc(sizeof(uint64_t) * 8);
	test_assert(buf != NULL);
	memset(buf, 0xAA, sizeof(uint64_t) * 8);

	model_allocator_checkpoint_next_force_full(&lp->mm_state);
	model_allocator_checkpoint_take(&lp->mm_state, 0);

	__write_mem(buf, sizeof(uint64_t) * 8);
	memset(buf, 0xBB, sizeof(uint64_t) * 8);
	model_allocator_checkpoint_take(&lp->mm_state, 1);

	__write_mem(buf, sizeof(uint64_t) * 8);
	memset(buf, 0xCC, sizeof(uint64_t) * 8);
	model_allocator_checkpoint_take(&lp->mm_state, 2);

	memset(buf, 0xFF, sizeof(uint64_t) * 8);
	model_allocator_checkpoint_restore(&lp->mm_state, 0);

	int errs = 0;
	for(int i = 0; i < 8; i++)
		errs += buf[i] != 0xAAAAAAAAAAAAAAAAULL;

	rs_free(buf);
	teardown_lp(lp);
	return errs > 0;
}

/**
 * After restore, ckpt_since_last_full and last_dirty_buddy must both be
 * reset regardless of how they were set before the restore.
 */
static int test_restore_resets_incremental_state(void *_)
{
	(void)_;
	struct lp_ctx *lp = setup_lp(true);
	global_config.full_ckpt_period = 100;

	void *p = rs_malloc(BLOCK_SIZE);
	test_assert(p != NULL);

	model_allocator_checkpoint_next_force_full(&lp->mm_state);
	model_allocator_checkpoint_take(&lp->mm_state, 0);

	for(int i = 1; i <= 3; i++) {
		__write_mem(p, BLOCK_SIZE);
		model_allocator_checkpoint_take(&lp->mm_state, (array_count_t)i);
	}
	test_assert(lp->mm_state.ckpt_since_last_full > 0);

	// Warm the last_dirty_buddy pointer
	__write_mem(p, BLOCK_SIZE);
	test_assert(lp->mm_state.last_dirty_buddy != NULL);

	model_allocator_checkpoint_restore(&lp->mm_state, 2);
	test_assert(lp->mm_state.ckpt_since_last_full == 0);
	test_assert(lp->mm_state.last_dirty_buddy == NULL);

	rs_free(p);
	teardown_lp(lp);
	return 0;
}

/* ============================================
 * Group G: model_allocator_fossil_lp_collect
 * ============================================ */

/**
 * fossil_lp_collect on a sequence of full checkpoints must retain all
 * checkpoints at or after the target, rebase their ref_idx by the freed
 * checkpoint's ref_idx, and return that ref_idx.
 */
static int test_fossil_collect_full_only(void *_)
{
	(void)_;
	struct lp_ctx *lp = setup_lp(false);

	void *p = rs_malloc(BLOCK_SIZE);
	test_assert(p != NULL);

	for(int i = 1; i <= 3; i++) {
		model_allocator_checkpoint_next_force_full(&lp->mm_state);
		model_allocator_checkpoint_take(&lp->mm_state, (array_count_t)(i * 10));
	}
	test_assert(array_count(lp->mm_state.logs) == 3);

	// Collect up to ref_idx 15: checkpoint at 10 qualifies
	array_count_t freed_ref = model_allocator_fossil_lp_collect(&lp->mm_state, 15);
	test_assert(freed_ref == 10);

	/*
	 * fossil_lp_collect rebases all retained logs by -ref_i (= -10) but
	 * does NOT remove the anchor log itself. All three logs survive;
	 * their ref_idx values are shifted by -10.
	 */
	test_assert(array_count(lp->mm_state.logs) == 3);
	test_assert(array_get_at(lp->mm_state.logs, 0).ref_idx == 0);  // was 10
	test_assert(array_get_at(lp->mm_state.logs, 1).ref_idx == 10); // was 20
	test_assert(array_get_at(lp->mm_state.logs, 2).ref_idx == 20); // was 30

	rs_free(p);
	teardown_lp(lp);
	return 0;
}

/**
 * fossil_lp_collect with an incremental chain must walk back to the full
 * checkpoint anchor: the full checkpoint must be retained even if the
 * target ref_idx falls within the incremental range.
 */
static int test_fossil_collect_incremental_anchor(void *_)
{
	(void)_;
	struct lp_ctx *lp = setup_lp(true);
	global_config.full_ckpt_period = 100;

	void *p = rs_malloc(BLOCK_SIZE);
	test_assert(p != NULL);

	// Full at 10, incremental at 20 and 30
	model_allocator_checkpoint_next_force_full(&lp->mm_state);
	model_allocator_checkpoint_take(&lp->mm_state, 10);

	__write_mem(p, BLOCK_SIZE);
	model_allocator_checkpoint_take(&lp->mm_state, 20);
	__write_mem(p, BLOCK_SIZE);
	model_allocator_checkpoint_take(&lp->mm_state, 30);

	test_assert(array_count(lp->mm_state.logs) == 3);

	/*
	 * Target 25: the last checkpoint at or before 25 is the incremental
	 * at 20. Must walk back to the full at 10, which must
	 * be retained as the chain anchor. All three logs survive.
	 */
	array_count_t freed_ref = model_allocator_fossil_lp_collect(&lp->mm_state, 25);
	test_assert(freed_ref == 10);
	test_assert(array_count(lp->mm_state.logs) == 3);

	rs_free(p);
	teardown_lp(lp);
	return 0;
}

/**
 * After fossil collection the retained chain must still allow a correct
 * restore.
 */
static int test_fossil_then_restore(void *_)
{
	(void)_;
	struct lp_ctx *lp = setup_lp(true);
	global_config.full_ckpt_period = 100;

	uint64_t *buf = rs_malloc(sizeof(uint64_t) * 4);
	test_assert(buf != NULL);

	// Full at 10 (AA)
	memset(buf, 0xAA, sizeof(uint64_t) * 4);
	model_allocator_checkpoint_next_force_full(&lp->mm_state);
	model_allocator_checkpoint_take(&lp->mm_state, 10);

	// Incremental at 20 (BB)
	__write_mem(buf, sizeof(uint64_t) * 4);
	memset(buf, 0xBB, sizeof(uint64_t) * 4);
	model_allocator_checkpoint_take(&lp->mm_state, 20);

	// Full at 30 (CC)
	model_allocator_checkpoint_next_force_full(&lp->mm_state);
	__write_mem(buf, sizeof(uint64_t) * 4);
	memset(buf, 0xCC, sizeof(uint64_t) * 4);
	model_allocator_checkpoint_take(&lp->mm_state, 30);

	/*
	 * GVT advances past 25: finds the incremental at 20,
	 * walks back to the full at 10 (anchor), and returns ref 10.
	 * fossil_lp_collect rebases all logs by -10 but keeps the anchor.
	 * All three checkpoints survive (full@0, incr@10, full@20 after rebase).
	 */
	array_count_t freed_ref = model_allocator_fossil_lp_collect(&lp->mm_state, 25);
	test_assert(freed_ref == 10);
	test_assert(array_count(lp->mm_state.logs) == 3);

	// The last log (full@30, rebased to 20) must not be incremental
	test_assert(!is_log_incremental(array_get_at(lp->mm_state.logs, array_count(lp->mm_state.logs) - 1)));

	// Overwrite and restore: buf must become CC. Rebased ref = 30 - freed_ref
	memset(buf, 0xFF, sizeof(uint64_t) * 4);
	model_allocator_checkpoint_restore(&lp->mm_state, 30 - (array_count_t)freed_ref);

	int errs = 0;
	for(int i = 0; i < 4; i++)
		errs += buf[i] != 0xCCCCCCCCCCCCCCCCULL;

	rs_free(buf);
	teardown_lp(lp);
	return errs > 0;
}

/* =================
 * Test entry point
 * ================= */

int model_allocator_full_test(_unused void *_)
{
	int errs = 0;

	// Group A
	errs += test_lp_init_fields(NULL);

	// Group B
	errs += test_malloc_zero(NULL);
	errs += test_malloc_single_block(NULL);
	errs += test_free_null(NULL);
	errs += test_find_by_address(NULL);
	errs += test_malloc_second_buddy(NULL);
	errs += test_full_ckpt_size_consistency(NULL);

	// Group C
	errs += test_calloc_zeroed(NULL);
	errs += test_calloc_marks_dirty(NULL);

	// Group D
	errs += test_realloc_null_ptr(NULL);
	errs += test_realloc_zero_size(NULL);
	errs += test_realloc_same_size_inplace(NULL);
	errs += test_realloc_copy_path(NULL);

	// Group E
	errs += test_full_checkpoint_restore(NULL);
	errs += test_full_multiple_checkpoints(NULL);
	errs += test_force_full_flag_reset(NULL);

	// Group F
	errs += test_incremental_smaller_than_full(NULL);
	errs += test_full_ckpt_period(NULL);
	errs += test_incremental_restore_mid_chain(NULL);
	errs += test_incremental_restore_to_full(NULL);
	errs += test_restore_resets_incremental_state(NULL);

	// Group G
	errs += test_fossil_collect_full_only(NULL);
	errs += test_fossil_collect_incremental_anchor(NULL);
	errs += test_fossil_then_restore(NULL);

	return errs > 0;
}
