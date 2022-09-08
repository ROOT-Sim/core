/**
 * @file lib/random/xxtea.c
 *
 * @brief XXTEA block cipher
 *
 * An implementation of the XXTEA block cipher
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <lib/random/xxtea.h>

#include <assert.h>

#define XXTEA_DELTA UINT32_C(0x9e3779b9)

static inline uint32_t xxtea_mx(uint32_t y, uint32_t z, uint32_t sum, unsigned p, unsigned e, const uint32_t key[4])
{
	return (((z >> 5 ^ y << 2) + (y >> 3 ^ z << 4)) ^ ((sum ^ y) + (key[(p & 3) ^ e] ^ z)));
}

void xxtea_encode(uint32_t *restrict v, unsigned n, uint32_t const key[restrict 4])
{
	assert(n > 1);
	uint32_t z = v[n - 1];
	unsigned rounds = 8 + 50 / n;
	uint32_t sum = XXTEA_DELTA;
	do {
		unsigned e = (sum >> 2) & 3;

		for(unsigned p = 0; p < n - 1; p++)
			z = v[p] += xxtea_mx(v[p + 1], z, sum, p, e, key);
		z = v[n - 1] += xxtea_mx(v[0], z, sum, n - 1, e, key);

		sum += XXTEA_DELTA;
	} while(--rounds);
}

void xxtea_decode(uint32_t *restrict v, unsigned n, uint32_t const key[restrict 4])
{
	assert(n > 1);
	uint32_t y = v[0];
	unsigned rounds = 8 + 50 / n;
	uint32_t sum = rounds * XXTEA_DELTA;
	do {
		unsigned e = (sum >> 2) & 3;

		for(unsigned p = n - 1; p > 0; p--)
			y = v[p] -= xxtea_mx(y, v[p - 1], sum, p, e, key);
		y = v[0] -= xxtea_mx(y, v[n - 1], sum, 0, e, key);

		sum -= XXTEA_DELTA;
	} while(--rounds);
}
