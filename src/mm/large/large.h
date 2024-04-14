#pragma once

// TODO: fossil collect old large allocation that have been freed

#include <stddef.h>

struct large_state {
	size_t s;
	unsigned char data[];
};

struct large_checkpoint {
	const struct large_state *orig;
	size_t s;
	unsigned char data[];
};

extern struct large_checkpoint *large_checkpoint_full_take(const struct large_state *self, struct large_checkpoint *data);
extern const void *large_checkpoint_full_restore(struct large_state *self, const struct large_checkpoint *data);
