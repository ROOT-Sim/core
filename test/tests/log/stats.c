/**
* @file test/tests/log/stats.c
*
* @brief Test: statistics module test
*
* SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
* SPDX-License-Identifier: GPL-3.0-only
*/
#include <log/stats.h>

#include <test.h>

#ifndef STATS_SCRIPT_PATH
#define STATS_SCRIPT_PATH ""
#endif

#define N_THREADS 2
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

void DummyProcessEvent(lp_id_t me, simtime_t now, unsigned event_type, const void *event_content, unsigned event_size,
    void *st)
{
	(void)me;
	(void)now;
	(void)event_type;
	(void)event_content;
	(void)event_size;
	(void)st;
}

bool DummyCanEnd(lp_id_t lid, const void *state)
{
	(void)lid;
	(void)state;
	return false;
}

static const struct simulation_configuration empty_conf = {
    .lps = 123,
    .n_threads = N_THREADS,
    .termination_time = 0,
    .gvt_period = 0,
    .log_level = LOG_SILENT,
    .stats_file = "stats_test",
    .ckpt_interval = 0,
    .prng_seed = 0,
    .core_binding = true,
    .serial = false,
    .dispatcher = DummyProcessEvent,
    .committed = DummyCanEnd,
};

static simtime_t gvt_tests[] = {0.0, 12.12, 23.4, 48.56};

test_ret_t stats_empty_test(__unused void *arg)
{
	stats_init();
	return 0;
}

test_ret_t stats_single_gvt_test(__unused void *arg)
{
	stats_init();
	stats_on_gvt(0.0);
	return 0;
}

test_ret_t stats_multi_gvt_test(__unused void *arg)
{
	stats_init();
	for(unsigned i = 0; i < sizeof(gvt_tests) / sizeof(*gvt_tests); ++i) {
		stats_on_gvt(gvt_tests[i]);
	}
	return 0;
}

test_ret_t stats_measure_test(__unused void *arg)
{
	stats_init();

	stats_take(STATS_ROLLBACK, 10);
	stats_take(STATS_MSG_PROCESSED, rid ? 53 : 73);
	stats_take(STATS_MSG_ROLLBACK, 12);
	stats_take(STATS_MSG_ANTI, 30);
	stats_take(STATS_MSG_SILENT, 15);

	stats_on_gvt(0.0);
	return 0;
}

static void stats_subsystem_test(const struct simulation_configuration *conf, test_fn thread_fn, const char *check_str)
{
	RootsimInit(conf);
	stats_global_init();
	parallel_test("Testing statistics module", thread_fn, NULL);
	stats_global_fini();

	char cmd[512] = {0};
	int ret;
	ret = snprintf(cmd, sizeof(cmd), "python3 " STATS_SCRIPT_PATH " %s.bin", conf->stats_file);
	test_assert(ret >= 0 && ret < (int)sizeof(cmd)); // assure correct snprintf operations
	ret = system(cmd);
	test_assert(ret == 0);

	ret = snprintf(cmd, sizeof(cmd), "python3 " STATS_TEST_SCRIPT_PATH " %s.txt %s", conf->stats_file, check_str);
	test_assert(ret >= 0 && ret < (int)sizeof(cmd)); // assure correct snprintf operations
	ret = system(cmd);
	test_assert(ret == 0);
}

int main(void)
{
	init(N_THREADS);

	stats_subsystem_test(&empty_conf, stats_empty_test,
	    "NZ 1 " TOSTRING(N_THREADS) " 0 0 0 0 0 0 0 0.00 0.00 100.00 0 0 0 0 0 0.0 0 0.0 0 NZ");

	n_lps_node = 16;
	stats_subsystem_test(&empty_conf, stats_single_gvt_test,
	    "NZ 1 " TOSTRING(N_THREADS) " 16 0 0 0 0 0 0 0.00 0.00 100.00 0 0 0 0 0 0.0 1 0.0 NZ NZ");

	stats_subsystem_test(&empty_conf, stats_multi_gvt_test,
	    "NZ 1 " TOSTRING(N_THREADS) " 16 0 0 0 0 0 0 0.00 0.00 100.00 0 0 0 0 0 48.56 4 12.14 NZ NZ");

	stats_subsystem_test(&empty_conf, stats_measure_test,
	    "NZ 1 " TOSTRING(N_THREADS) " 16 156 102 24 30 20 60 15.87 1.20 80.95 0 0 0 0 0 0.0 1 0.0 NZ NZ");

	// TODO: stress test the sample aggregation system

	finish();
}
