#include <test.h>

static int fail_test(void)
{
	return -1;
}

const struct _test_config_t test_config = {
	.test_name = "fail_fini",
	.test_fini_fnc = fail_test
};
