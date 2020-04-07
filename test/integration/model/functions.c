#include <integration/model/application.h>

#include <string.h>
#include <stdio.h>

static uint32_t super_fast_hash(const char *data, unsigned len);

buffer* get_buffer(buffer *head, unsigned i)
{
	while(i--) {
		head = head->next;
	}
	return head;
}

uint32_t read_buffer(buffer *head, unsigned i)
{
	head = get_buffer(head, i);
	return super_fast_hash((char *)head->data, head->count * sizeof(uint64_t));
}

buffer* allocate_buffer(lp_state_t *state, unsigned *data, unsigned count)
{

	buffer *new = malloc(sizeof(buffer) + count * sizeof(uint64_t));
	new->next = state->head;
	new->count = count;

	if (data != NULL) {
		memcpy(new->data, data, count * sizeof(uint64_t));
	} else {
		for(unsigned i = 0; i < count; i++) {
			new->data[i] = lcg_random_u(state->rng_state);
		}
	}

	return new;
}

buffer* deallocate_buffer(buffer *head, unsigned i)
{
	buffer *prev = NULL;
	buffer *to_free = head;

	for (unsigned j = 0; j < i; j++) {
		prev = to_free;
		to_free = to_free->next;
	}

	if (prev != NULL) {
		prev->next = to_free->next;
		free(to_free);
		return head;
	}

	prev = head->next;
	free(head);
	return prev;
}

// The following code is a stripped version of the one found in http://www.azillionmonkeys.com/qed/hash.html
// Provided by Paul Hsieh under license LGPL 2.1 http://www.gnu.org/licenses/lgpl-2.1.txt

#define get16bits(d) (*((const uint16_t *) (d)))

static uint32_t super_fast_hash(const char *data, unsigned len)
{
	uint32_t hash = len, tmp;
	unsigned rem;

	rem = len & 3;
	len >>= 2;

	/* Main loop */
	while(len--) {
		hash  += get16bits(data);
		tmp    = (get16bits(data + 2) << 11) ^ hash;
		hash   = (hash << 16) ^ tmp;
		data  += 2 * sizeof(uint16_t);
		hash  += hash >> 11;
	}

	/* Handle end cases */
	switch (rem) {
		case 3:
			hash += get16bits (data);
			hash ^= hash << 16;
			hash ^= data[sizeof(uint16_t)] << 18;
			hash += hash >> 11;
			break;
		case 2:
			hash += get16bits(data);
			hash ^= hash << 11;
			hash += hash >> 17;
			break;
		case 1:
			hash += *data;
			hash ^= hash << 10;
			hash += hash >> 1;
	}

	/* Force "avalanching" of final 127 bits */
	hash ^= hash << 3;
	hash += hash >> 5;
	hash ^= hash << 4;
	hash += hash >> 17;
	hash ^= hash << 25;
	hash += hash >> 6;

	return hash;
}
