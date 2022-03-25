/**
* @file test/self-tests/rng.c
*
* @brief Test: Test for the rng used in tests
*
* A simple test to verify statistical properties of the random number generator used in test programs
*
* SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
* SPDX-License-Identifier: GPL-3.0-only
*/

#include <stdint.h>
#include <math.h>

#include <test.h>

/* Perform the Kolmogorov-Smirnov test on the uniform random number
 * generator.
 */
static int ks_test(uint32_t N, uint32_t nBins)
{
	uint32_t i, index, cumulativeSum;
	double rf, ksThreshold, countPerBin;
	uint32_t bins[nBins];

	// Fill the bins
	for (i = 0; i < nBins; i++)
		bins[i] = 0;

	for (i = 0; i < N; i++) {
		rf = test_random();
		index = floor(rf * nBins);
		if (index >= nBins) // just in case...
			index = nBins - 1;

		bins[index]++;
	}

	// Test the bins
	ksThreshold = 1.358 / sqrt((double)N);
	countPerBin = (double)N / nBins;
	cumulativeSum = 0;
	for (i = 0; i < nBins; i++) {
		cumulativeSum += bins[i];
		if((double) cumulativeSum / N - (i + 1) * countPerBin / N >= ksThreshold)
			return 1;
	}
	return 0;
}

test_ret_t test_rng(__unused void *_)
{
	int passed = 0;

	passed += ks_test(1000000, 1000);
	passed += ks_test(100000, 1000);
	passed += ks_test(10000, 100);
	passed += ks_test(1000, 10);
	passed += ks_test(100, 10);

	return passed;
}
