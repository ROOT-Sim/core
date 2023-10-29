#include <mm/buddy/reverse.h>

#include <distributed/distributed_mem.h>
#include <lp/lp.h>

#include <stdalign.h>

#define REVERSE_BLOCK_EXP 3U
#define REVERSE_MASK ((1U << REVERSE_BLOCK_EXP) - 1)

_Static_assert(alignof(uintptr_t) <= (1U << REVERSE_BLOCK_EXP), "The memory block for write tracking is under-aligned");

union reverse_op {
	struct {
		uintptr_t info;
		struct buddy_state *buddy;
	} alloc;

	struct {
		uintptr_t *position;
		uintptr_t value;
	} write;

	struct {
		uintptr_t count;
		uintptr_t mem_size;
	} last;
};

static _Thread_local array_declare(union reverse_op) reverse_log;

void *reverse_message_done(void)
{
	union reverse_op last = {
	    .last = {
		.count = array_count(reverse_log),
		.mem_size = current_lp->mm_state.full_ckpt_size
	    }
	};
	array_push(reverse_log, last);
	void *ret = array_items(reverse_log);
	array_count_t cap = array_capacity(reverse_log);
	array_init_explicit(reverse_log, cap);
	return ret;
}

void mark_memory_will_write(const void *ptr, size_t s)
{
	if(!distributed_mem_is_contained(ptr))
		return;

	uintptr_t up = (uintptr_t)ptr;
	s += up & REVERSE_MASK;
	up &= ~(uintptr_t)REVERSE_MASK;
	s += REVERSE_MASK;
	s >>= REVERSE_BLOCK_EXP;
	uintptr_t *p = (uintptr_t *)up;

	while(s--)
		array_push(reverse_log, ((union reverse_op){.write = {.position = &p[s], .value = p[s]}}));
}

void reverse_allocator_operation_register(struct buddy_state *buddy, uint_fast16_t info)
{
	array_push(reverse_log, ((union reverse_op){.alloc = {.buddy = buddy, .info = info}}));
}

void reverse_restore(void *checkpoint)
{
	union reverse_op *chk = checkpoint;
	for(array_count_t i = chk->last.count; i--;) {
		--chk;
		if(likely(chk->alloc.info > UINT16_MAX))
			*chk->write.position = chk->write.value;
		else
			buddy_operation_reverse(chk->alloc.buddy, chk->alloc.info);
	}
}
