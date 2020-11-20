#include <test.h>
#include <mm/msg_allocator.h>

static int msg_allocator_test(void)
{
	msg_allocator_init();
	// TODO ACTUAL TEST
	msg_allocator_fini();
	return 0;
}

const struct _test_config_t test_config = {
	.threads_count = 4,
	.test_fnc = msg_allocator_test
};
