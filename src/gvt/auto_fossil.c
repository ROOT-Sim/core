#include <gvt/auto_fossil.h>

#include <log/stats.h>

__thread simtime_t auto_fossil_threshold;
simtime_t auto_fossil_threshold_init;

enum auto_fossil_state {
	FORWARD_DOUBLING_STEP = 0,
	BACKWARD_HALVING_STEP
};

__thread struct {
	uint64_t last_cost;
	simtime_t step;
	enum auto_fossil_state state;
} auto_fossil_ctx;

void auto_fossil_init(void) {
	auto_fossil_threshold = auto_fossil_threshold_init;
	auto_fossil_ctx.last_cost = UINT64_MAX;
	auto_fossil_ctx.step = 1.0;
	auto_fossil_ctx.state = FORWARD_DOUBLING_STEP;
}

void auto_fossil_on_gvt(void)
{
	return;
	uint64_t current_cost = stats_retrieve(STATS_ROLLBACK_TIME) + stats_retrieve(STATS_FOSSIL_TIME);
	switch (auto_fossil_ctx.state) {
		case FORWARD_DOUBLING_STEP:
			if(current_cost > auto_fossil_ctx.last_cost) {
				auto_fossil_ctx.step /= 2;
				auto_fossil_threshold -= auto_fossil_ctx.step;
				auto_fossil_ctx.state = BACKWARD_HALVING_STEP;
			} else {
				auto_fossil_ctx.step *= 2;
				auto_fossil_threshold += auto_fossil_ctx.step;
			}
			break;
		case BACKWARD_HALVING_STEP:
			if(current_cost < auto_fossil_ctx.last_cost) {
				auto_fossil_ctx.step /= 2;
				auto_fossil_threshold -= auto_fossil_ctx.step;
			} else {
				auto_fossil_ctx.step *= 2;
				auto_fossil_threshold += auto_fossil_ctx.step;
				auto_fossil_ctx.state =  FORWARD_DOUBLING_STEP;
			}
			break;
	}
	auto_fossil_ctx.last_cost = current_cost;
}
