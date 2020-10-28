#include <core/sync.h>

#include <core/core.h>

static atomic_uint barr_in;
static atomic_uint barr_out;
static atomic_uint barr_cr;

#ifdef NEUROME_TEST
#define BARRIER_IN_THRESHOLD (1024)
#else
#define BARRIER_IN_THRESHOLD (UINT_MAX/2)
#endif

bool sync_thread_barrier(void)
{
	unsigned i;
	const unsigned count = n_threads;
	const unsigned max_in_before_reset = BARRIER_IN_THRESHOLD - BARRIER_IN_THRESHOLD % count;
	do {
		i = atomic_fetch_add_explicit(
			&barr_in, 1U, memory_order_acq_rel) + 1;
	} while (unlikely(i > max_in_before_reset));

	unsigned cr = atomic_load_explicit(&barr_cr, memory_order_relaxed);

	bool leader = i == cr + count;
	if (leader) {
		atomic_store_explicit(
			&barr_cr, cr + count, memory_order_release);
	} else {
		while (i > cr) {
			cr = atomic_load_explicit (&barr_cr, memory_order_relaxed);
			spin_pause();
		}
	}
	atomic_thread_fence(memory_order_acquire);

	unsigned o = atomic_fetch_add_explicit(&barr_out, 1, memory_order_release) + 1;
	if (unlikely(o == max_in_before_reset)) {
		atomic_thread_fence(memory_order_acquire);
		atomic_store_explicit(&barr_cr, 0, memory_order_relaxed);
		atomic_store_explicit(&barr_out, 0, memory_order_relaxed);
		atomic_store_explicit(&barr_in, 0, memory_order_release);
	}

	return leader;
}
