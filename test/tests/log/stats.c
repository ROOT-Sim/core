#include <log/stats.h>

#include <test.h>

#define N_THREADS 2

void DummyProcessEvent(lp_id_t me, simtime_t now, unsigned event_type, const void *event_content,
    unsigned event_size, void *st)
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

struct simulation_configuration conf = {
    .lps = 16,
    .n_threads = N_THREADS,
    .termination_time = 1000,
    .gvt_period = 1000,
    .log_level = LOG_INFO,
    .stats_file = "stats_test",
    .ckpt_interval = 0,
    .prng_seed = 0,
    .core_binding = true,
    .serial = false,
    .dispatcher = DummyProcessEvent,
    .committed = DummyCanEnd,
};

test_ret_t stats_test(__unused void *arg)
{
	stats_init();
	stats_on_gvt(0.0);
}

int main(void)
{
	init(N_THREADS);
	RootsimInit(&conf);
	stats_global_init();
	parallel_test("Testing statistics module", stats_test, NULL);

	stats_global_fini();

	//TODO: write actual test cases

	stats_global_init();
	parallel_test("Testing statistics module", stats_test, NULL);
	stats_global_fini();

	finish();
}