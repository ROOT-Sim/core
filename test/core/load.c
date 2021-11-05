#include <stdlib.h>
#include <stdio.h>

#include <ROOT-Sim.h>
#include "test.h"

void foo() {}

struct simulation_configuration conf = {
    .lps = 0,
    .dispatcher = NULL,
    .committed = NULL,
};

int main(void)
{
	init();

	test_xf("LPs not set", RootsimInit, &conf);
	conf.lps = 1;
	test_xf("Handler not set", RootsimInit, &conf);
	conf.dispatcher = (ProcessEvent_t)foo;
	test_xf("CanEnd not set", RootsimInit, &conf);
	conf.committed = (CanEnd_t)foo;
	test("Initialization", RootsimInit, &conf);

	finish();
}
