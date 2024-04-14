#include <mm/large/large.h>

#include <core/core.h>

#include <string.h>

struct large_checkpoint *large_checkpoint_full_take(const struct large_state *self, struct large_checkpoint *data)
{
	data->orig = self;
	void *p = &data->s;
	size_t cs = offsetof(struct large_state, data) + self->s;
	memcpy(p, self, cs);
	return ((char *)p) + cs;
}

const void *large_checkpoint_full_restore(struct large_state *self, const struct large_checkpoint *data)
{
	if(unlikely(data->orig != self))
		return NULL;

	const void *p = &data->s;
	size_t cs = offsetof(struct large_state, data) + data->s;
	memcpy(self, p, cs);
	return ((const char *)p) + cs;
}
