#include <lp/lp.h>

__thread lp_struct *current_lp;
extern lp_struct *lps;
extern uint64_t lps_count;

void lp_global_init(uint64_t lps_cnt)
{
	lps = malloc(sizeof(*lps) * lps_cnt);
	lps_count = lps_cnt;
}

void lp_global_fini(void)
{
	free(lps);
}

void lp_process_msg(lp_msg *msg)
{

}
