#include <lib/state/state.h>

#include <lib/lib_internal.h>

void SetState(void *state)
{
	lib_state_managed.state_s = state;
}

void state_lib_lp_init(void)
{
	lib_state_managed.state_s = NULL;
}
