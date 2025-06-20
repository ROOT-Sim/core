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

/// A subset of LPs in the Karmarkar-Karp algorithm
struct subset {
	/// The weight of the subset
	weight_type weight;
	/// A pointer to the first lp that belongs to this subset
	/** This effectively is the head of a list of LPs, which uses the next member of struct lp **/
	struct lp_ctx *head;
	/// A pointer to the next member of the last LP in this list, to speed-up list appends
	struct lp_ctx **tail_p;
};

/// A (potentially still partial) partitioning of LPs in the Karmarkar-Karp algorithm
struct partitioning {
	/// The difference in weight between the heaviest and the lightest subset in this partitioning
	weight_type differencing;
	/// An array of k subsets (where k is the number of worker threads) that form the partitioning
	struct subset *subsets;
};

///
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
static double binding_threshold_score = -DBL_MAX;

static _Atomic weight_type weight_total = 0, weight_max = 0;

/**
 * @brief Update the score of a binding
 * @param weight_local the weight of the LP subset installed on the current thread
 *
 * This function is meant to be called by all the workers thread. Once this has done, binding_score_compute() can be
 * called to get the binding score
 */
static inline void binding_score_update(weight_type weight_local)
{
	weight_type c = 0;
	while(c < weight_local && !atomic_compare_exchange_weak_explicit(&weight_max, &c, weight_local,
				     memory_order_relaxed, memory_order_relaxed))
		spin_pause();
	atomic_fetch_add_explicit(&weight_total, weight_local, memory_order_relaxed);
}

/**
 * @brief Compute the score of a binding
 * @return the score of a binding, a positive number
 *
 * This function can be called meaningfully only after binding_score_update() has been called for all the installed
 * subsets. To compute the score for a new binding, binding_score_reset() must be called at least once
 */
static inline double binding_score_compute(void) {
	weight_type wtot = atomic_load_explicit(&weight_total, memory_order_relaxed);
	weight_type wmax = atomic_load_explicit(&weight_max, memory_order_relaxed);
	assert(wmax * global_config.n_threads >= wtot);
	return wtot != 0 ? (double)(wmax * global_config.n_threads) / (double)wtot - 1.0 : 0.0;
}

/**
 * @brief Reset the score computation of a binding
 *
 * Should be used to reset the internal state of a binding score computation.
 */
static inline void binding_score_reset(void) {
	atomic_store_explicit(&weight_total, 0, memory_order_relaxed);
	atomic_store_explicit(&weight_max, 0, memory_order_relaxed);
}

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
	heap_heapify_from_array(partitionings, partitioning_cmp);

	unsigned k = global_config.n_threads;
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
	if(sync_thread_barrier()) {
		karmarkar_karp_run();
		binding_score_reset();
	}

	// barrier needed to make sure threads do not access the output data before the single thread phase completed
	sync_thread_barrier();

	// refresh the binding score and apply the binding
	struct subset *assigned_subset = &heap_min(partitionings).subsets[tid];
	binding_score_update(assigned_subset->weight);
	thread_first_lp = assigned_subset->head;
	for(struct lp_ctx *lp = thread_first_lp; lp; lp = lp->next)
		lp->rid = tid;

	// barrier needed to make sure threads applied the new binding and updated their cost
	if(sync_thread_barrier()) {
		// one lucky thread does the last bit of cleanup
		mm_free(heap_min(partitionings).subsets);
		heap_count(partitionings) = 0;
		binding_threshold_score = binding_score_compute() + REBINDING_SCORE_THRESHOLD;
	}

	msg_queue_local_rebind();

	// barrier needed otherwise the GVT algorithm may miss some of the messages that are being rebound in the queues
	sync_thread_barrier();
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
	switch(rebind_thread_phase) {
		case rebind_thread_phase_A:
			{
				weight_type w = 0;
				for(struct lp_ctx *lp = thread_first_lp; lp; lp = lp->next)
					w += lp->cost;

				binding_score_update(w);
			}
			break;

		case rebind_thread_phase_B:
			// TODO: improve decision criteria for rebinding
			if(binding_score_compute() > binding_threshold_score)
				rebind_compute();
			// Update moving average; dividing only the last estimate will make it in average
			// 8 times bigger than the correct estimation, but everything is proportional so we don't care
			for(struct lp_ctx *lp = thread_first_lp; lp; lp = lp->next)
				lp->cost = lp->cost * 7 / 8;
			break;

		case rebind_thread_phase_C:
			if(tid == global_config.n_threads - 1)
				binding_score_reset();
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
