#include <rebind/rebind.h>

#include <core/sync.h>
#include <datatypes/heap.h>
#include <datatypes/msg_queue.h>
#include <gvt/gvt.h>

#define partitioning_cmp(a, b) ((a).differencing > (b).differencing)
#define REBINDING_SCORE_THRESHOLD 0.05

typedef timer_uint weight_type;

enum rebind_thread_phase {
	rebind_thread_phase_idle = 0,
	rebind_thread_phase_A,
	rebind_thread_phase_B,
	rebind_thread_phase_C,
};

struct subset {
	weight_type weight;
	struct lp_ctx *head;
	struct lp_ctx **tail_p;
};

struct partitioning {
	weight_type differencing;
	struct subset *subsets;
};

struct {
	lp_id_t capacity;
	union {
		lp_id_t count;
		_Atomic lp_id_t atomic_count;
	};
	struct partitioning *items;
} partitionings;

static __thread enum rebind_thread_phase rebind_thread_phase = rebind_thread_phase_idle;

static atomic_flag rebind_do_reset;
static _Atomic tid_t rebind_counter;
static double binding_last_score = -DBL_MAX;


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

static int partitioning_qsort_cmp(const void *a, const void *b)
{
	const struct partitioning *p1 = a, *p2 = b;
	return (p1->differencing < p2->differencing) - (p1->differencing > p2->differencing);
}

static void rebind_compute(void)
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

	if(sync_thread_barrier()) {
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

	sync_thread_barrier();

	thread_first_lp = heap_min(partitionings).subsets[tid].head;
	for(struct lp_ctx *lp = thread_first_lp; lp; lp = lp->next)
		lp->rid = tid;

	sync_thread_barrier();

	msg_queue_local_rebind();

	// leaks heap_min(partitionings).subsets, freed in rebind_phase_run()
}

void rebind_global_init(void)
{
	array_init_explicit(partitionings, global_config.lps);
	atomic_store_explicit(&rebind_counter, global_config.n_threads * (global_config.rebind_check_each_gvt + 1), memory_order_relaxed);
}

void rebind_global_fini(void)
{
	array_fini(partitionings);
}

#define update_score(total_var, max_var)                                                                               \
	__extension__({                                                                                                \
		weight_type w = 0, c = 0;                                                                              \
		for(struct lp_ctx *lp = thread_first_lp; lp; lp = lp->next)                                            \
			w += lp->cost;                                                                                 \
                                                                                                                       \
		while(c < w && !atomic_compare_exchange_weak_explicit(&max_var, &c, w, memory_order_relaxed,           \
				   memory_order_relaxed))                                                              \
			spin_pause();                                                                                  \
		atomic_fetch_add_explicit(&total_var, w, memory_order_relaxed) == 0;                                   \
	})

#define score_compute(total_var, max_var)                                                                              \
	__extension__({                                                                                                \
		weight_type wtot = atomic_load_explicit(&total_var, memory_order_relaxed);                             \
		weight_type wmax = atomic_load_explicit(&max_var, memory_order_relaxed);                               \
                                                                                                                       \
                assert(wmax * global_config.n_threads >= wtot);                                                        \
		wtot != 0 ? (double)(wmax * global_config.n_threads) / (double)wtot - 1.0 : 0.0;                       \
	})

static void rebind_phase_run(void)
{
	static _Atomic weight_type current_total = 0, current_max = 0;
	static _Atomic weight_type last_total, last_max;
	switch(rebind_thread_phase) {
		case rebind_thread_phase_A:
			if(update_score(current_total, current_max)) {
				atomic_store_explicit(&last_total, 0U, memory_order_relaxed);
				atomic_store_explicit(&last_max, 0U, memory_order_relaxed);
				atomic_flag_clear_explicit(&rebind_do_reset, memory_order_relaxed);
			}
			rebind_thread_phase = rebind_thread_phase_B;
			break;
		case rebind_thread_phase_B:
			if(score_compute(current_total, current_max) > binding_last_score + REBINDING_SCORE_THRESHOLD) {
				rebind_compute();
				update_score(last_total, last_max);
			}
			// Update moving average
			for(struct lp_ctx *lp = thread_first_lp; lp; lp = lp->next)
				lp->cost = lp->cost * 7 / 8;

			atomic_fetch_add_explicit(&rebind_counter, global_config.rebind_check_each_gvt + 1, memory_order_relaxed);
			rebind_thread_phase = rebind_thread_phase_C;
			break;
		case rebind_thread_phase_C:
			if(!atomic_flag_test_and_set_explicit(&rebind_do_reset, memory_order_relaxed)) {
				if(!heap_is_empty(partitionings)) {
					binding_last_score = score_compute(last_total, last_max);
					printf("\n%lf\n", binding_last_score);
					mm_free(heap_min(partitionings).subsets);
					heap_count(partitionings) = 0;
				}

				atomic_store_explicit(&current_total, 0U, memory_order_relaxed);
				atomic_store_explicit(&current_max, 0U, memory_order_relaxed);
			}
			rebind_thread_phase = rebind_thread_phase_idle;
			break;
		default:
			__builtin_unreachable();
	}
}

void rebind_on_gvt(void)
{
	if(unlikely(rebind_thread_phase)) {
		rebind_phase_run();
	} else if(atomic_fetch_sub_explicit(&rebind_counter, 1U, memory_order_relaxed) <= global_config.n_threads){
		rebind_thread_phase = rebind_thread_phase_A;
		rebind_phase_run();
	}
}
