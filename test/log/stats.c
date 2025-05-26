/**
 * @file test/tests/log/stats.c
 *
 * @brief Test: statistics module test
 *
 * SPDX-FileCopyrightText: 2008-2025 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <log/stats.h>

#include <test.h>

#define N_THREADS 2

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

static struct simulation_configuration conf = {
    .lps = 123,
    .n_threads = N_THREADS,
    .termination_time = 0,
    .gvt_period = 0,
    .log_level = LOG_SILENT,
    .stats_file = NULL,
    .ckpt_interval = 0,
    .core_binding = true,
    .serial = false,
    .dispatcher = DummyProcessEvent,
    .committed = DummyCanEnd,
};

static simtime_t gvt_tests[] = {0.0, 12.12, 23.4, 48.56};

int stats_empty_test(_unused void *arg)
{
	rid = test_parallel_thread_id();
	stats_init();
	return 0;
}

int stats_single_gvt_test(_unused void *arg)
{
	rid = test_parallel_thread_id();
	stats_init();
	stats_on_gvt(0.0);
	return 0;
}

int stats_multi_gvt_test(_unused void *arg)
{
	rid = test_parallel_thread_id();
	stats_init();
	for(unsigned i = 0; i < sizeof(gvt_tests) / sizeof(*gvt_tests); ++i)
		stats_on_gvt(gvt_tests[i]);
	return 0;
}

int stats_measures_test(_unused void *arg)
{
	rid = test_parallel_thread_id();
	stats_init();

	stats_take(STATS_ROLLBACK, 10);
	stats_take(STATS_MSG_PROCESSED, rid ? 53 : 73);
	stats_take(STATS_MSG_ROLLBACK, 12);
	stats_take(STATS_MSG_ANTI, 30);
	stats_take(STATS_MSG_SILENT, 15);

	stats_on_gvt(0.0);
	return 0;
}

static void stats_subsystem_test(const char *name, test_fn thread_fn)
{
	conf.stats_file = name;
	RootsimInit(&conf);
	stats_global_init();
	test_parallel("Testing statistics module", thread_fn, NULL, N_THREADS);
	stats_global_fini();
}

int main(void)
{
	stats_subsystem_test("empty_stats", stats_empty_test);
	n_lps_node = 16;
	stats_subsystem_test("single_gvt_stats", stats_single_gvt_test);
	stats_subsystem_test("multi_gvt_stats", stats_multi_gvt_test);
	stats_subsystem_test("measures_stats", stats_measures_test);

	// TODO: stress test the sample aggregation system
}
