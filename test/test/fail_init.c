#include <test.h>

static int fail_test(void)
{
	return -1;
}

const struct _test_config_t test_config = {
	.test_name = "fail init",
	.test_init_fnc = fail_test
};
