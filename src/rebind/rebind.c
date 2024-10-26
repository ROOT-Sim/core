#include <rebind/rebind.h>

#include <core/sync.h>
#include <datatypes/heap.h>
#include <datatypes/msg_queue.h>
#include <gvt/gvt.h>

#define partitioning_cmp(a, b) ((a).differencing > (b).differencing)
#define REBINDING_SCORE_THRESHOLD 0.05

typedef timer_uint weight_type;

/// A thread phase during the local rebinding computation; ordered in reverse because they are used in a countdown
enum rebind_thread_phase {
	/// The last phase of rebinding: some minor cleanup and reset of the countdown
	rebind_thread_phase_C = 0,
	/// The main phase of rebinding: carry out synchronously the actual rebinding if necessary
	rebind_thread_phase_B,
	/// The initial phase of rebinding: update variables necessary to update the local score
	rebind_thread_phase_A,
	/// The count of rebind phases, used to reset the countdown
	rebind_thread_phase_count
};

// TODO: document
struct subset {
	weight_type weight;
	struct lp_ctx *head;
	struct lp_ctx **tail_p;
};

// TODO: document
struct partitioning {
	weight_type differencing;
	struct subset *subsets;
};

// TODO: document
struct {
	lp_id_t capacity;
	union {
		lp_id_t count;
		_Atomic lp_id_t atomic_count;
	};
	struct partitioning *items;
} partitionings;

/// This is effectively used as a countdown, where the last values map to rebind phase that need to be carried out
static __thread enum rebind_thread_phase rebind_thread_phase;
/// The score of the last binding; this initialization value force the rebinding to act at least once
static double binding_last_score = -DBL_MAX;

// TODO: document
#define update_score(local_total, total_var, max_var)                                                                  \
	__extension__({                                                                                                \
		weight_type c = 0;                                                                                     \
		while(c < local_total && !atomic_compare_exchange_weak_explicit(&max_var, &c, local_total,             \
					     memory_order_relaxed, memory_order_relaxed))                              \
			spin_pause();                                                                                  \
		atomic_fetch_add_explicit(&total_var, local_total, memory_order_relaxed);                              \
	})

// TODO: document
#define score_compute(total_var, max_var)                                                                              \
	__extension__({                                                                                                \
		weight_type wtot = atomic_load_explicit(&total_var, memory_order_relaxed);                             \
		weight_type wmax = atomic_load_explicit(&max_var, memory_order_relaxed);                               \
                                                                                                                       \
		assert(wmax *global_config.n_threads >= wtot);                                                         \
		wtot != 0 ? (double)(wmax * global_config.n_threads) / (double)wtot - 1.0 : 0.0;                       \
	})

/**
 * @brief Update the state of partial partitioning after a merge
 *
 * Stuff specific to the Karmarkar-Karp algorithm
 */
static void partitioning_update(unsigned k, struct partitioning *p)
{
	// sort the subsets; insertion sort should be enough for our purpose (expecting k <= 128)
	struct subset *s = p->subsets;
	for(unsigned i = 1; i < k; ++i) {
		struct subset c = s[i];
		unsigned j = i - 1;

		while(j < k && c.weight > s[j].weight) {
			s[j + 1] = s[j];
			j--;
		}
		s[j + 1] = c;
	}
	// update the differencing
	p->differencing = p->subsets[0].weight - p->subsets[k - 1].weight;
}

/**
 * @brief Compare two partial partitionings
 *
 * Will cause the qsort to order the partitionings from the highest differencing to the lowest one
 */
static int partitioning_qsort_cmp(const void *a, const void *b)
{
	const struct partitioning *p1 = a, *p2 = b;
	return (p1->differencing < p2->differencing) - (p1->differencing > p2->differencing);
}

/**
 * @brief Initialize the state for the Karmarkar-Karp heuristic partition algorithm
 *
 * This function is meant to be called by all the threads participating in the computation
 */
static void karmarkar_karp_init(void)
{
	unsigned k = global_config.n_threads;
	for(struct lp_ctx *lp = thread_first_lp; lp;) {
		// causes memory consumption to be O(k * w_count). Oh well!
		// TODO: avoid alloc and free, allocate once at startup or implement O(w_count) allocation method
		struct subset *subsets = mm_alloc(sizeof(*subsets) * k);

		// initialize first elem
		subsets->head = lp;
		subsets->tail_p = &lp->next;
		subsets->weight = lp->cost;

		memset(subsets + 1, 0, sizeof(*subsets) * (k - 1));

		struct partitioning partitioning = {.differencing = subsets->weight, .subsets = subsets};

		lp_id_t i = atomic_fetch_add_explicit(&partitionings.atomic_count, 1U, memory_order_relaxed);
		partitionings.items[i] = partitioning;

		struct lp_ctx *next = lp->next;
		lp->next = NULL;
		lp = next;
	}
}

/**
 * @brief Run the Karmarkar-Karp heuristic partition algorithm
 *
 * This function is meant to be called by a single thread. There's no easy way to parallelize this code without
 * sacrificing the quality of the solution. Suggestions accepted!
 */
static void karmarkar_karp_run(void)
{
	unsigned k = global_config.n_threads;
	qsort(partitionings.items, partitionings.count, sizeof(*partitionings.items), partitioning_qsort_cmp);

	do {
		struct partitioning p1 = heap_extract(partitionings, partitioning_cmp);
		struct partitioning p2 = heap_extract(partitionings, partitioning_cmp);

		for(unsigned m = 0; m < k; ++m) {
			struct subset *s1 = &p1.subsets[m];
			struct subset *s2 = &p2.subsets[k - m - 1];
			if(s2->head) {
				s1->weight += s2->weight;
				if(s1->head)
					*s1->tail_p = s2->head;
				else
					s1->head = s2->head;
				s1->tail_p = s2->tail_p;
			}
		}
		mm_free(p2.subsets);
		partitioning_update(k, &p1);
		heap_insert(partitionings, partitioning_cmp, p1);
	} while(heap_count(partitionings) > 1);
}

/**
 * @brief Run the actual synchronous rebinding algorithm
 *
 * This is expensive, so we only want to execute it if necessary
 */
static void rebind_compute(void)
{
	karmarkar_karp_init();

	// barrier needed to make sure all threads completed the initialization
	if(sync_thread_barrier())
		karmarkar_karp_run();

	// barrier needed to make sure threads do not access the output data before the single thread phase completed
	sync_thread_barrier();

	// apply the binding and compute the new local cost
	thread_first_lp = heap_min(partitionings).subsets[tid].head;
	weight_type w = 0;
	for(struct lp_ctx *lp = thread_first_lp; lp; lp = lp->next) {
		w += lp->cost;
		lp->rid = tid;
	}

	static _Atomic weight_type total_cost = 0, max_local_cost = 0;
	update_score(w, total_cost, max_local_cost);

	// barrier needed to make sure threads applied the new binding and updated their cost
	if(sync_thread_barrier()) {
		// one lucky thread does the last bit of cleanup
		mm_free(heap_min(partitionings).subsets);
		heap_count(partitionings) = 0;
	}

	msg_queue_local_rebind();

	// barrier needed to get rid of a rare bug with the GVT, that effectively otherwise cannot see the messages in
	// flight while the messages are being rebound in the queues
	if(sync_thread_barrier()) {
		// one lucky thread updates the binding score
		binding_last_score = score_compute(total_cost, max_local_cost) + REBINDING_SCORE_THRESHOLD;
		atomic_store_explicit(&total_cost, 0U, memory_order_relaxed);
		atomic_store_explicit(&max_local_cost, 0U, memory_order_relaxed);
	}
}

/**
 * @brief Initialize the rebinding subsystem at the rank level
 */
void rebind_global_init(void)
{
	array_init_explicit(partitionings, global_config.lps);
}

/**
 * @brief Initialize the rebinding subsystem at the thread level
 */
void rebind_init(void)
{
	rebind_thread_phase = rebind_thread_phase_count + global_config.rebind_gvt_interval;
}

/**
 * @brief Finalize the rebinding subsystem at the rank level
 */
void rebind_global_fini(void)
{
	array_fini(partitionings);
}

/**
 * @brief Execute a phase of the rebinding computation
 */
static void rebind_phase_run(void)
{
	static _Atomic weight_type total_cost = 0, max_local_cost = 0;
	switch(rebind_thread_phase) {
		case rebind_thread_phase_A:
			{
				weight_type w = 0;
				for(struct lp_ctx *lp = thread_first_lp; lp; lp = lp->next)
					w += lp->cost;

				update_score(w, total_cost, max_local_cost);
			}
			break;

		case rebind_thread_phase_B:
			// TODO: improve decision criteria for rebinding
			if(score_compute(total_cost, max_local_cost) > binding_last_score)
				rebind_compute();
			// Update moving average; clearly dividing only the last estimate will make it in average
			// 4 times bigger than the correct estimation, but everything is proportional so we don't care
			for(struct lp_ctx *lp = thread_first_lp; lp; lp = lp->next)
				lp->cost = lp->cost * 7 / 8;
			break;

		case rebind_thread_phase_C:
			if(tid == global_config.n_threads - 1) {
				atomic_store_explicit(&total_cost, 0U, memory_order_relaxed);
				atomic_store_explicit(&max_local_cost, 0U, memory_order_relaxed);
			}
			// reset countdown
			rebind_thread_phase = rebind_thread_phase_count + global_config.rebind_gvt_interval;
			break;
		default:
			__builtin_unreachable();
	}
}

/**
 * @brief Execute a phase of the rebinding computation at gvt time, if necessary
 */
void rebind_on_gvt(void)
{
	if(unlikely(--rebind_thread_phase < rebind_thread_phase_count))
		rebind_phase_run();
}
