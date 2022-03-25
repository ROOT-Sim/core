/**
* @file test/framework/rng.c
*
* @brief RNG for the test framework
*
* Ultra-fast RNG: Use a fast hash of integers. 2^64 Period. Passes Diehard and TestU01 at maximum settings.
*
* SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
* SPDX-License-Identifier: GPL-3.0-only
 */
#include <time.h>

#include <test.h>

static _Thread_local unsigned long long rnd_seed;

// Robert Jenkins' mix function
static unsigned long long mix64(unsigned long long a, unsigned long long b, unsigned long long c)
{
	a -= b; a -= c; a ^= (c>>43);
	b -= c; b -= a; b ^= (a<<9);
	c -= a; c -= b; c ^= (b>>8);
	a -= b; a -= c; a ^= (c>>38);
	b -= c; b -= a; b ^= (a<<23);
	c -= a; c -= b; c ^= (b>>5);
	a -= b; a -= c; a ^= (c>>35);
	b -= c; b -= a; b ^= (a<<49);
	c -= a; c -= b; c ^= (b>>11);
	a -= b; a -= c; a ^= (c>>12);
	b -= c; b -= a; b ^= (a<<18);
	c -= a; c -= b; c ^= (b>>22);
	return c;
}


unsigned int test_random(void)
{
	unsigned long long c = 7319936632422683443ULL;
	unsigned long long x = (rnd_seed += c);

	x ^= x >> 32;
	x *= c;
	x ^= x >> 32;
	x *= c;
	x ^= x >> 32;

	/* Return lower 32bits */
	return x;
}


// This function must be explicitly called on each thread.
// Currently, it's called in init() and upon the creation of every worker thread.
void test_random_init(void)
{
	rnd_seed = mix64(clock(), time(NULL), getpid());
}
