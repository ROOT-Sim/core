#include <mm/model_allocator.h>

#include <lp/lp.h>
#include <core/intrinsics.h>

#include <stdlib.h>
#include <errno.h>

#define FULL_LOG_THRESHOLD 0.7

#define left_child(i) (((i) << 1U) + 1U)
#define right_child(i) (((i) << 1U) + 2U)
#define parent(i) ((((i) + 1) >> 1U) - 1U)
#define is_power_of_2(i) (!((i) & ((i) - 1)))
#define next_exp_of_2(i) (sizeof(i) * CHAR_BIT - SAFE_CLZ(i))

#ifdef HAVE_COVERAGE
extern void *__real_malloc(size_t mem_size);
extern void *__real_realloc(void *ptr, size_t mem_size);
extern void __real_free(void *ptr);
#endif

void model_memory_lp_init(void)
{
	struct mm_state *self = &current_lp->mm_state;
	uint_fast8_t node_size = B_TOTAL_EXP;

	for (uint_fast32_t i = 0;
		i < sizeof(self->longest) / sizeof(*self->longest); ++i) {
		self->longest[i] = node_size;
		node_size -= is_power_of_2(i + 2);
	}

	self->used_mem = 0;
	self->base_mem = mm_alloc(1 << B_TOTAL_EXP);
}

void model_memory_lp_fini(void)
{
	mm_free(current_lp->mm_state.base_mem);
}

void *__wrap_malloc(size_t req_size)
{
	if(unlikely(!req_size))
		return NULL;

#ifdef HAVE_COVERAGE
	if(unlikely(!current_lp)){
		return __real_malloc(req_size);
	}
#endif

	struct mm_state *self = &current_lp->mm_state;

	uint_fast8_t req_blks = max(next_exp_of_2(req_size - 1), B_BLOCK_EXP);

	if (self->longest[0] < req_blks) {
		errno = ENOMEM;
		return NULL;
	}

	/* search recursively for the child */
	uint_fast8_t node_size;
	uint_fast32_t i;
	for (
		i = 0, node_size = B_TOTAL_EXP;
		node_size > req_blks;
		--node_size
	) {
		/* choose the child with smaller longest value which
		 * is still large at least *size* */
		i = left_child(i);
		i += self->longest[i] < req_blks;
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

void *__wrap_calloc(size_t nmemb, size_t size)
{
	size_t tot = nmemb * size;
	void *ret = __wrap_malloc(tot);

	if(ret)
		memset(ret, 0, tot);

	return ret;
}

void __wrap_free(void *ptr)
{
	if(unlikely(!ptr))
		return;

#ifdef HAVE_COVERAGE
	if(unlikely(!current_lp)){
		__real_free(ptr);
		return;
	}
#endif

	struct mm_state *self = &current_lp->mm_state;
	uint_fast8_t node_size = B_BLOCK_EXP;
	uint_fast32_t i =
		(((uintptr_t)ptr - (uintptr_t)self->base_mem) >> B_BLOCK_EXP) +
		(1 << (B_TOTAL_EXP - B_BLOCK_EXP)) - 1;

	for (; self->longest[i]; i = parent(i)) {
		++node_size;
	}

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

void *__wrap_realloc(void *ptr, size_t req_size)
{
	if(!req_size){
		__wrap_free(ptr);
		return NULL;
	}
	if(!ptr){
		return __wrap_malloc(req_size);
	}

	abort();
	return NULL;
}

mm_checkpoint *model_checkpoint_take(void)
{
	struct mm_state *self = &current_lp->mm_state;
	mm_checkpoint *ret;
	//if(self->used_mem > FULL_LOG_THRESHOLD * (1 << B_TOTAL_EXP)){
		ret = mm_alloc(
			offsetof(mm_checkpoint, base_mem) + (1 << B_TOTAL_EXP));
		ret->used_mem = self->used_mem;
		memcpy(ret->longest, self->longest, sizeof(ret->longest));
		memcpy(ret->base_mem, self->base_mem, 1 << B_TOTAL_EXP);
		return ret;
	//}
	// todo partial log

	return NULL;
}

void model_checkpoint_restore(mm_checkpoint *ckp)
{
	struct mm_state *self = &current_lp->mm_state;
	//if(ckp->used_mem > FULL_LOG_THRESHOLD * (1 << B_TOTAL_EXP)){
		self->used_mem = ckp->used_mem;
		memcpy(self->longest, ckp->longest, sizeof(ckp->longest));
		memcpy(self->base_mem, ckp->base_mem, 1 << B_TOTAL_EXP);
		return;
	//}
	// todo partial restore
}

void model_checkpoint_free(mm_checkpoint *ckp)
{
	mm_free(ckp);
}
