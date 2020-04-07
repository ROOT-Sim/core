#include <lib/random/random.h>

#include <lib/lib_internal.h>

#include <lib/random/xoroshiro.h>

void random_lib_lp_init(uint64_t llid)
{
	random_init(lib_state_managed.rng_s, llid);
}

double Random(void)
{
	return random_u01(lib_state_managed.rng_s);
}

uint64_t RandomU64(void)
{
	return random_u64(lib_state_managed.rng_s);
}
