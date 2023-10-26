#include <distributed//distributed_mem.h>

#include <arch/mem.h>
#include <core/core.h>
#include <distributed/mpi.h>
#include <mm/mm.h>

_Static_assert(MEM_DETERMINISTIC_PAGE_SIZE % sizeof(struct distr_mem_chunk) == 0,
    "Distributed memory chunks are not page aligned");

static struct distr_mem_chunk *all_chunks, *start_chunks, *end_chunks;
static size_t distr_mem_size;
static _Atomic(void *) *chunks_refs;
static _Thread_local uint_fast64_t chunk_id;

#define distr_mem_total_size_compute()                                                                                 \
	__extension__({                                                                                                \
		size_t ret = max(global_config.bytes_per_lp, sizeof(struct distr_mem_chunk)) * global_config.lps;      \
		size_t grain = MEM_DETERMINISTIC_PAGE_SIZE * n_nodes;                                                  \
		ret = (ret + grain - 1) / grain * grain;                                                               \
		ret;                                                                                                   \
	})

void distributed_mem_global_init(void)
{
	uintptr_t ptr = 0x100000000U;
	distr_mem_size = distr_mem_total_size_compute();
	size_t chunks_count = distr_mem_size / sizeof(struct distr_mem_chunk);
	chunks_refs = mm_alloc(sizeof(*chunks_refs) * chunks_count);
	memset(chunks_refs, 0, sizeof(*chunks_refs) * chunks_count);

	if(global_config.serial) {
		all_chunks = mem_aligned_alloc(alignof(struct distr_mem_chunk), distr_mem_size);
		start_chunks = all_chunks;
		end_chunks = all_chunks + chunks_count;
		return;
	}

	while(true) {
		while(mem_deterministic_alloc((void *)ptr, distr_mem_size) != 0)
			ptr += distr_mem_size;

		uintptr_t ptr_max = ptr;
		mpi_blocking_reduce_u64_max(&ptr_max);

		bool fail = false;
		if(ptr != ptr_max) {
			mem_deterministic_free((void *)ptr, distr_mem_size);
			fail = mem_deterministic_alloc((void *)ptr_max, distr_mem_size) != 0;
		}

		uintptr_t fail_any = fail;
		mpi_blocking_reduce_u64_max(&fail_any);

		if(!fail_any)
			break;

		if(!fail)
			mem_deterministic_free((void *)ptr_max, distr_mem_size);

		ptr = ptr_max + distr_mem_size;
	}

	all_chunks = (struct distr_mem_chunk *)ptr;
	start_chunks = all_chunks + chunks_count * nid / n_nodes;
	end_chunks = all_chunks + chunks_count * (nid + 1) / n_nodes;
}

void distributed_mem_global_fini(void)
{
	if(global_config.serial)
		mem_aligned_free(all_chunks);
	else
		mem_deterministic_free(all_chunks, distr_mem_size);
	mm_free(chunks_refs);
}

struct distr_mem_chunk *distributed_mem_chunk_alloc(void *ref)
{
	uint_fast64_t start_id = chunk_id;
	do {
		if(&start_chunks[chunk_id] == end_chunks)
			chunk_id = 0;

		void *nptr = NULL;
		if(atomic_compare_exchange_strong_explicit(&chunks_refs[&start_chunks[chunk_id] - all_chunks], &nptr,
		       ref, memory_order_relaxed, memory_order_relaxed))
			return &start_chunks[chunk_id++];

	} while(++chunk_id != start_id);
	return NULL;
}

void distributed_mem_chunk_free(struct distr_mem_chunk *chk)
{
	atomic_store_explicit(&chunks_refs[chk - all_chunks], NULL, memory_order_relaxed);
}

void *distributed_mem_chunk_ref(const struct distr_mem_chunk *chk)
{
	return atomic_load_explicit(&chunks_refs[chk - all_chunks], memory_order_relaxed);
}

extern bool distributed_mem_is_contained(const void *ptr)
{
 	return (uintptr_t)ptr - (uintptr_t)all_chunks < distr_mem_size;
}
