#include <ROOT-Sim.h>

#include "control_msg.h"

struct simulation_configuration conf = {
    .lps = 16,
    .n_threads = 0,
    .serial = true,
    .dispatcher = (ProcessEvent_t)ProcessEvent,
    .committed = (CanEnd_t)1,
};

int main(void)
{
	RootsimInit(&conf);
	return test_ctrl_msg(false);
}
