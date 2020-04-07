#include <mm/msg_allocator.h>

#include <stddef.h>
#include <stdlib.h>

#include <core/core.h>
#include <datatypes/array.h>

static __thread dyn_array(lp_msg *) free_list = {0};

void msg_allocator_init(void)
{
	array_init(free_list);
}

void msg_allocator_fini(void)
{
	while(!array_is_empty(free_list)){
		free(array_pop(free_list));
	}
	array_fini(free_list);
}

lp_msg* msg_allocator_alloc(unsigned payload_size)
{
	lp_msg *ret;
	if(unlikely(payload_size > BASE_PAYLOAD_SIZE)){
		ret = malloc(
			offsetof(lp_msg, extra_pl) +
			(payload_size - BASE_PAYLOAD_SIZE)
		);
		ret->pl_size = payload_size;
		return ret;
	}
	if(unlikely(array_is_empty(free_list))){
		ret = malloc(sizeof(lp_msg));
		ret->pl_size = payload_size;
		return ret;
	}
	return array_pop(free_list);
}

void msg_allocator_free(lp_msg *msg)
{
	if(likely(msg->pl_size <= BASE_PAYLOAD_SIZE))
		array_push(free_list, msg);
	else
		free(msg);
}

extern lp_msg* msg_allocator_pack(unsigned receiver, simtime_t timestamp,
	unsigned event_type, const void *payload, unsigned payload_size);
