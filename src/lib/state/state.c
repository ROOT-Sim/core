#include <lib/state/state.h>

#include <lib/lib_internal.h>

void SetState(void *state)
{
	struct lib_state_managed *lsm = l_s_m_p;
	mark_written(&lsm->state_s, sizeof(lsm->state_s));
	lsm->state_s = state;
}

void state_lib_lp_init(void)
{
	l_s_m_p->state_s = NULL;
}
