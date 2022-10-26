/*************************************************************************
 * prioq.c
 *
 * Lock-free priority queue based on linked list implementation of Harris' algorithm
 *   "A Pragmatic Implementation of Non-Blocking Linked Lists" 
 *   T. Harris, p. 300-314, DISC 2001.
 *
 */


#include <assert.h>

/* keir fraser's garbage collection */
#include "../msg_queue_nb_skip_list/gc/ptst.h"

/* some utilities (e.g. memory barriers) */
#include "nb_linked_list.h"

/* thread state. */
extern __thread ptst_t *ptst;

static int gc_id;

#define SENTINEL_KEYMIN (-1.0)        /* Key value of first dummy node. */
#define SENTINEL_KEYMAX (SIMTIME_MAX) /* Key value of last dummy node.  */

#define get_marked_ref(_p) ((void *)(((uintptr_t)(_p)) | 1))
#define get_unmarked_ref(_p) ((void *)(((uintptr_t)(_p)) & ~1))
#define is_marked_ref(_p) (((uintptr_t)(_p)) & 1)

enum nb_ll_op {
    NB_LL_PQ_DELETEMIN,
    NB_LL_PQ_PEEKMIN
};


/* initialize new node */
static nb_ll_node_t *alloc_node(void)
{
    nb_ll_node_t *n;
    n = gc_alloc(ptst, gc_id);
    memset(n, 0, sizeof(nb_ll_node_t));
    return n;
}


/* Mark node as ready for reclamation to the garbage collector. */
static void free_node(nb_ll_node_t *n)
{
    gc_free(ptst, (void *)n, gc_id);
}


__thread unsigned long long traversed = 0;
__thread unsigned long long ins = 0;

/***** search *****/
static nb_ll_node_t *search(nb_ll_pq_t *pq, pkey_t k, nb_ll_node_t **pred, nb_ll_node_t **succ)
{
    nb_ll_node_t *l, *l_next, *r, *r_next;

    l = pq->head;
    l_next = l->next;
    l_next = get_unmarked_ref(l_next);

    assert(is_marked_ref(l_next) == 0);
    assert(l_next != NULL);
    
    r = l;
    r_next = l_next;
    while(r->k <= k && r != pq->tail) {
        if(!is_marked_ref(r_next)){
            l = r;
            l_next = r_next;
        }
        r = get_unmarked_ref(r_next);
        r_next = r->next;
        traversed++;
        assert(r != NULL);
    }
    *pred = l;
    *succ = r;
    assert(l != NULL && l->k <= k);
    assert(r != NULL && r->k > k);
    assert(!is_marked_ref(l_next));

    return l_next;
}

static void release_disconnected_nodes(nb_ll_node_t *l, nb_ll_node_t *r){
    nb_ll_node_t *l_next = l->next;
    while(l != r) {
        assert(is_marked_ref(l_next));
        free_node(l);
        l = get_unmarked_ref(l_next);
        l_next = l->next;
        assert(is_marked_ref(l_next) || l == r);
    }
    assert(l == r);
}

/***** insert *****/
void nb_ll_pq_insert(nb_ll_pq_t *pq, pkey_t k, pval_t v)
{
    nb_ll_node_t *l, *l_next, *r, *new;
    assert(SENTINEL_KEYMIN < k && k < SENTINEL_KEYMAX);
    critical_enter();

    /* Initialise a new node for insertion. */
    new = alloc_node();
    new->k = k;
    new->v = v;

    /* lowest level insertion retry loop */
retry:
    l_next = search(pq, k, &l, &r);
    assert(!is_marked_ref(l_next));
    assert(l->k <= k && k < r->k);
    ins++;

    new->next = r;
    
    if(!__sync_bool_compare_and_swap(&l->next, l_next, new))
        goto retry;

    if(l_next != r)
        release_disconnected_nodes(l_next,r);
    
    IWMB(); /* this flag must be reset after all CAS have completed */
    critical_exit();
    
    if((ins % 50000) == 0) printf("\n\nINS %llu %llu\n\n", traversed, traversed/ins);


}

__thread unsigned long long skipped = 0;
__thread unsigned long long ops = 0;

static void do_operation(enum nb_ll_op operation, nb_ll_pq_t *pq, pkey_t *k, pval_t *v){
    nb_ll_node_t *cur;
    nb_ll_node_t *cur_next;
    nb_ll_node_t *old_next;
    nb_ll_node_t *head_next;
    nb_ll_node_t *last_to_be_kept;
    int count;
    int attempt;    

    critical_enter();

  begin:
    count = 0;
    attempt = 0;
    cur = head_next = pq->head->next;
    
    while(cur != pq->tail){
        cur_next = cur->next;
        if(!is_marked_ref(cur_next)){

            if(operation == NB_LL_PQ_PEEKMIN){
                if(count > pq->max_offset && __sync_bool_compare_and_swap(&pq->head->next, head_next, cur))
                    release_disconnected_nodes(head_next,cur);
                break;
            }    
            else if(operation == NB_LL_PQ_DELETEMIN){
                if(++attempt == 1 && count > pq->max_offset && __sync_bool_compare_and_swap(&pq->head->next, head_next, cur)){
                    release_disconnected_nodes(head_next,cur);
                    count = 0;
                    head_next = cur;
                }
                if(__sync_bool_compare_and_swap(&cur->next, cur_next, get_marked_ref(cur_next))) {
                    last_to_be_kept = cur_next;
                    break;
                }
            }
            else 
                continue;
            
        }
        count++;
        cur = get_unmarked_ref(cur_next);

        if(head_next != pq->head->next) goto begin; // a new min has been inserted so restart
    }
    *k = cur->k;
    *v = cur->v;
    skipped += count;
    ops += 1;
    if((ops % 50000) == 0) printf("\n\nEXT %lld %lld %lld\n\n", skipped,count, skipped/ops);
    critical_exit();
}

pkey_t nb_ll_pq_peek_key(nb_ll_pq_t *pq)
{
    pkey_t r_k;
    pval_t r_v;
    do_operation(NB_LL_PQ_PEEKMIN, pq, &r_k, &r_v);
    assert(r_v != NULL || r_k == SENTINEL_KEYMAX );
    assert(r_k != SENTINEL_KEYMIN);
    return r_k;
}


pval_t nb_ll_pq_deletemin(nb_ll_pq_t *pq)
{
    pkey_t r_k;
    pval_t r_v;
    do_operation(NB_LL_PQ_DELETEMIN, pq, &r_k, &r_v);
    assert(r_v != NULL || r_k == SENTINEL_KEYMAX );
    assert(r_k != SENTINEL_KEYMIN);
    return r_v;
}

/* Init structure, setup sentinel head and tail nodes. */
void nb_ll_pq_init(nb_ll_pq_t *pq, int max_offset)
{
    pq->tail = malloc(sizeof(nb_ll_node_t));
    pq->tail->k = SENTINEL_KEYMAX;
    pq->tail->v = NULL;
    pq->tail->next = NULL;

    pq->head = malloc(sizeof(nb_ll_node_t));
    pq->head->k = SENTINEL_KEYMIN;
    pq->head->v = NULL;
    pq->head->next = pq->tail;

    pq->max_offset = max_offset;
}

void _nb_ll_pq_init_allocators(void)
{
    _init_gc_subsystem();
    gc_id = gc_add_allocator(sizeof(nb_ll_node_t));
}

/* Cleanup, mark all the nodes for recycling. */
void nb_ll_pq_destroy(nb_ll_pq_t *pq)
{
    nb_ll_node_t *cur, *pred;
    cur = pq->head;
    while(cur != pq->tail) {
        pred = cur;
        cur = get_unmarked_ref(pred->next);
        free_node(pred);
    }
    free(pq->tail);
    free(pq->head);
}
