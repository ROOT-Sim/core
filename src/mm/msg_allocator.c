#include <mm/msg_allocator.h>

#include <stddef.h>
#include <stdlib.h>

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

lp_msg* msg_alloc(unsigned payload_size)
{
	lp_msg *ret;
	if(unlikely(payload_size > FIXED_EVENT_PAYLOAD)){
		ret = malloc(offsetof(lp_msg, additional_payload) + (payload_size - FIXED_EVENT_PAYLOAD) * sizeof(unsigned char));
		ret->payload_size = payload_size;
		return ret;
	}
	if(unlikely(array_is_empty(free_list))){
		ret = malloc(sizeof(lp_msg));
		ret->payload_size = payload_size;
		return ret;
	}
	return array_pop(free_list);
}

void msg_free(lp_msg *msg)
{
	if(likely(msg->payload_size <= FIXED_EVENT_PAYLOAD))
		array_push(free_list, msg);
	else
		free(msg);
}


