#include <mm/ckpt_interval.h>

#include <log/stats.h>
#include <lp/process.h>

#include <math.h>

static _Thread_local struct {
	double c_c;
	double ir_p;
	double ic_s;
} h;

void ckpt_on_gvt(void)
{
	if (global_config.ckpt_interval)
		return;

	uint64_t ckpt_cost = stats_retrieve(STATS_CKPT_TIME);
	uint64_t ckpt_count = stats_retrieve(STATS_CKPT);
	uint64_t rb_count = stats_retrieve(STATS_ROLLBACK);
	uint64_t msg_count = stats_retrieve(STATS_MSG_PROCESSED);
	uint64_t sil_count = stats_retrieve(STATS_MSG_SILENT);
	uint64_t sil_cost = stats_retrieve(STATS_MSG_SILENT_TIME);

	unsigned rate;
	if (likely(ckpt_count && sil_count)) {
		h.c_c = (double)ckpt_cost / (double)ckpt_count + 3.0 * h.c_c;
		h.ir_p = (double)msg_count / (double)rb_count + 3.0 * h.ir_p;
		h.ic_s = (double)sil_count / (double)sil_cost + 3.0 * h.ic_s;
		h.c_c *= 0.25;
		h.ir_p *= 0.25;
		h.ic_s *= 0.25;
		rate = ceil(sqrt(fabs(2 * h.ir_p * h.ic_s * h.c_c)));
	} else {
		rate = 1024;
	}

	process_ckpt_rate_set(rate);
}
