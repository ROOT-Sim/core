#include <lib/state/state.h>

#include <lib/lib_internal.h>

void SetState(void *state)
{
	lib_state_managed.state_s = state;
}
