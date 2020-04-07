#include <NeuRome.h>

#include <test_rng.h>

#include <memory.h>

#define MAX_BUFFERS 256
#define MAX_BUFFER_SIZE 512
#define SEND_PROBABILITY 0.05
#define ALLOC_PROBABILITY 0.2
#define DEALLOC_PROBABILITY 0.2
#define COMPLETE_EVENTS 10000

enum {
	LOOP = INIT + 1,
	RECEIVE
};

typedef struct _buffer {
	unsigned count;
	struct _buffer *next;
	uint64_t data[];
} buffer;

typedef struct _lp_state_type {
	unsigned events;
	unsigned buffer_count;
	uint32_t total_checksum;
	uint128_t rng_state;
	buffer *head;
} lp_state_t;

buffer* get_buffer(buffer *head, unsigned i);
uint32_t read_buffer(buffer *head, unsigned i);
buffer* allocate_buffer(lp_state_t *state, unsigned *data, unsigned count);
buffer* deallocate_buffer(buffer *head, unsigned i);
