#include <lib/random/random.h>

#include <core/intrinsics.h>
#include <lib/lib_internal.h>
#include <lib/random/xoroshiro.h>

#include <math.h>
#include <memory.h>


void random_lib_lp_init(void)
{
	uint64_t lid = current_lid;
	struct lib_state_managed *lsm = l_s_m_p;
	random_init(lsm->rng_s, lid);
	lsm->unif = NAN;
}

double Random(void)
{
	struct lib_state_managed *lsm = l_s_m_p;
	uint64_t u_val = random_u64(lsm->rng_s);
	if (unlikely(!u_val)) {
		return 0.0;
	}

	double ret = 0.0;
	unsigned lzs = SAFE_CLZ(u_val) + 1;
	u_val <<= lzs;
	u_val >>= 12;

	uint64_t exp = 1023 - lzs;
	u_val |= exp << 52;

	memcpy(&ret, &u_val, sizeof(double));
	return ret;
}

uint64_t RandomU64(void)
{
	struct lib_state_managed *lsm = l_s_m_p;
	mark_written(&lsm->rng_s, sizeof(lsm->rng_s));
	return random_u64(lsm->rng_s);
}

double Expent(double mean)
{
	if (unlikely(mean < 0)) {
		log_log(LOG_WARN, "Passed a negative mean into Expent()");
	}
	return -mean * log(1 - Random());
}

double Normal(void)
{
	struct lib_state_managed *lsm = l_s_m_p;
	if (isnan(lsm->unif)) {
		double v1, v2, rsq;
		do {
			v1 = 2.0 * Random() - 1.0;
			v2 = 2.0 * Random() - 1.0;
			rsq = v1 * v1 + v2 * v2;
		} while (rsq >= 1.0 || rsq == 0);

		double fac = sqrt(-2.0 * log(rsq) / rsq);

		// Perform Box-Muller transformation to get two normal deviates. Return one
		// and save the other for next time.
		lsm->unif = v1 * fac;
		return v2 * fac;
	} else {
		// A deviate is already available
		double ret = lsm->unif;
		lsm->unif = NAN;
		return ret;
	}
}

