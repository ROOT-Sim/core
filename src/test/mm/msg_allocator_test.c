#include <test/test.h>
#include <mm/msg_allocator.h>

static int msg_allocator_test(unsigned thread_id)
{
	(void)thread_id;
	msg_allocator_init();
	// TODO ACTUAL TEST
	msg_allocator_fini();
	return 0;
}

struct _test_config_t test_config = {
	.test_name = "message allocator",
	.threads_count = 4,
	.test_fnc = msg_allocator_test
};
