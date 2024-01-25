#include <datatypes/msg_skip_list.h>

#include <core/intrinsics.h>

static _Thread_local uint64_t skiplist_rng_state = 125;

static uint64_t random_next(void) {
	uint64_t z = (skiplist_rng_state += 0x9e3779b97f4a7c15);
//	z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
	z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
	return z ^ (z >> 31);
}

static void skip_list_expand(struct msg_skip_list *msl)
{
	msl->capacity *= 2;
	msl->nodes = mm_realloc(msl->nodes + 1, sizeof(*msl->nodes) * msl->capacity);
	--msl->nodes;
	for(array_count_t i = 1 + msl->capacity / 2; i < msl->capacity; ++i)
		msl->nodes[i].next[0] = i + 1;
	msl->nodes[msl->capacity].next[0] = 0;
	msl->free_list = 1 + msl->capacity / 2;
}

void msg_skip_list_init(struct msg_skip_list *msl)
{
	msl->capacity = 8;
	msl->nodes = mm_alloc(sizeof(*msl->nodes) * msl->capacity);
	--msl->nodes;
	for(array_count_t i = 1; i < msl->capacity; ++i)
		msl->nodes[i].next[0] = i + 1;
	msl->nodes[msl->capacity].next[0] = 0;
	msl->free_list = 1;
	memset(msl->next, 0, sizeof(msl->next));
}

void msg_skip_list_fini(struct msg_skip_list *msl)
{
	++msl->nodes;
	mm_free(msl->nodes);
}

void msg_skip_list_insert(struct msg_skip_list *msl, struct lp_msg *msg)
{
	if(unlikely(msl->free_list == 0))
		skip_list_expand(msl);

	uint64_t r = random_next() | (1U << MSG_SKIP_LIST_MAX_HEIGHT);
	unsigned new_l = intrinsics_ctz(r);

	struct msg_skip_list_node *n = msl->nodes;
	struct msg_skip_list_node *insert = &msl->nodes[msl->free_list];
	msl->free_list = insert->next[0];

	simtime_t t = msg->dest_t;
	insert->msg = msg;
	insert->msg_t = t;

	array_count_t *s = msl->next;
	unsigned l = MSG_SKIP_LIST_MAX_HEIGHT - 1;
	for(; l > new_l; --l) {
		while(s[l] && n[s[l]].msg_t < t)
			s = n[s[l]].next;
	}

	do {
		while(s[l] && n[s[l]].msg_t < t)
			s = n[s[l]].next;

		insert->next[l] = s[l];
		s[l] = insert - n;
	} while(l--);

}

struct lp_msg *msg_skip_list_extract(struct msg_skip_list *msl)
{
	array_count_t *s = msl->next;
	array_count_t first_i = s[0];
	if(unlikely(first_i == 0))
		return NULL;

	struct msg_skip_list_node *f = &msl->nodes[first_i];
	s[0] = f->next[0];
	for(unsigned l = 1; l < MSG_SKIP_LIST_MAX_HEIGHT; ++l) {
		if(s[l] != first_i)
			break;
		s[l] = f->next[l];
	}

	f->next[0] = msl->free_list;
	msl->free_list = first_i;

	return f->msg;
}

struct lp_msg *msg_skip_list_try_anti(struct msg_skip_list *msl, const struct lp_msg *msg)
{
	uint64_t id = msg->raw_flags - MSG_FLAG_ANTI;
	simtime_t t = msg->dest_t;
	struct msg_skip_list_node *n = msl->nodes;
	array_count_t *s = msl->next;
	array_count_t update[MSG_SKIP_LIST_MAX_HEIGHT];

	unsigned l = MSG_SKIP_LIST_MAX_HEIGHT;
	do {
		--l;
		while (s[l] && (n[s[l]].msg_t < t || (s[l] && n[s[l]].msg_t == t && n[s[l]].msg->raw_flags != id)))
			s = n[s[l]].next;

		update[l] = s[l];
	} while(--l);

	array_count_t current = update[0];
	if (!current || n[current].msg->raw_flags != id)
		return NULL;

	for (unsigned l = 0; l < MSG_SKIP_LIST_MAX_HEIGHT; ++l) {
		if (n[current].next[l] != current)
			s[l] = n[current].next[l];
		else
			s[l] = update[l];
	}

	n[current].next[0] = msl->free_list;
	msl->free_list = current;
	return n[current].msg;
}
