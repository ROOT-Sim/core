#include <test.h>

#include <core/init.h>
#include <core/arg_parse.h>

void log_logo_print(void){}

bool log_colored;

char *args_lp_wt_1[] = {
	"init_test",
	"--wt",
	"2",
	"--lp",
	"64",
	NULL
};

char *args_lp_wt_2[] = {
	"init_test",
	"--wt",
	"1",
	"--lp",
	"80",
	NULL
};

char *args_no_bind[] = {
	"init_test",
	"--wt",
	"10",
	"--lp",
	"40",
	"--no-bind",
	NULL
};

char *args_gvt[] = {
	"init_test",
	"--wt",
	"10",
	"--lp",
	"40",
	"--gvt-period",
	"500",
	NULL
};

char *args_termination[] = {
	"init_test",
	"--wt",
	"2",
	"--lp",
	"64",
	"--time",
	"1437.23",
	NULL
};

#define TEST_INIT(args_arr, cond) 					\
__extension__({								\
	init_args_parse(sizeof(args_arr) / sizeof(*(args_arr)) - 1,	\
		(args_arr));						\
	if (!(cond)) return -1;						\
})

int main(int argc, char **argv)
{
	(void) argc;
	(void) argv;
	TEST_INIT(args_lp_wt_1,
		n_lps == 64 && n_threads == 2 && global_config.core_binding
		&& global_config.gvt_period == 200000
		&& global_config.termination_time == SIMTIME_MAX);

	TEST_INIT(args_lp_wt_2,
		n_lps == 80 && n_threads == 1 && global_config.core_binding);

	TEST_INIT(args_no_bind,
		n_lps == 40 && n_threads == 10 && !global_config.core_binding);

	TEST_INIT(args_gvt,
		n_lps == 40 && n_threads == 10 && global_config.core_binding
		&& global_config.gvt_period == 500000);

	TEST_INIT(args_termination,
		n_lps == 64 && n_threads == 2 && global_config.core_binding
		&& global_config.gvt_period == 200000
		&& global_config.termination_time == 1437.23);

	test_printf("test done");
	return 0;
}

const struct _test_config_t test_config = {
	.expected_output = "test done",
	.expected_output_size = sizeof("test done") - 1
};
