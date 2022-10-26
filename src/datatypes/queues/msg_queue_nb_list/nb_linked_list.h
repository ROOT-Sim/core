#ifndef NB_LINKED_LIST_H
#define NB_LINKED_LIST_H

#include <core/core.h>
#include "../msg_queue_nb_skip_list/common.h"

typedef simtime_t pkey_t;
typedef void *pval_t;

typedef struct nb_ll_node_s {
    pkey_t k;
    pval_t v;
    struct nb_ll_node_s *volatile next;
} nb_ll_node_t;

typedef struct {
    nb_ll_node_t *head;
    nb_ll_node_t *tail;
    int max_offset;
} nb_ll_pq_t;


extern void _nb_ll_pq_init_allocators(void);

extern void nb_ll_pq_init(nb_ll_pq_t *pq, int max_offset);
extern void nb_ll_pq_destroy(nb_ll_pq_t *pq);

extern void nb_ll_pq_insert(nb_ll_pq_t *pq, pkey_t k, pval_t v);

extern pval_t nb_ll_pq_deletemin(nb_ll_pq_t *pq);
extern pkey_t nb_ll_pq_peek_key(nb_ll_pq_t *pq);

extern void nb_ll_pq_sequential_length(nb_ll_pq_t *pq);


#endif // PRIOQ_H
