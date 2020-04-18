#include <lib/random/random.h>

#include <lib/lib_internal.h>

#include <lib/random/xoroshiro.h>

void random_lib_lp_init(void)
{
	uint64_t lid = current_lid;
	random_init(lib_state_managed.rng_s, lid);
}

double Random(void)
{
	return random_u01(lib_state_managed.rng_s);
}

uint64_t RandomU64(void)
{
	return random_u64(lib_state_managed.rng_s);
}

double Expent(double mean)
{
	if (unlikely(mean < 0)) {
		log_log(LOG_WARN, "Passed a negative mean into Expent()");
	}
	return -mean * log(1 - Random());
}
