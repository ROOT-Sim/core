/**
 * @file test/tests/lib/xxtea.c
 *
 * @brief Test: XXTEA cipher
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <test.h>

#include <lib/random/xxtea.h>

#include <stdlib.h>
#include <string.h>

#define MAX_BLOCKS 1024
#define TRIES 1024

int xxtea_test(_unused void *args)
{
	uint32_t key[4] = {test_random_u() >> 32, test_random_u() >> 32, test_random_u() >> 32, test_random_u() >> 32};

	uint32_t(*data)[MAX_BLOCKS] = malloc(MAX_BLOCKS * sizeof(*data));
	uint32_t(*data_enc)[MAX_BLOCKS] = malloc(MAX_BLOCKS * sizeof(*data_enc));
	if(data == NULL || data_enc == NULL)
		return -1;

	memset(data, 0, sizeof(*data));
	for(unsigned i = TRIES; i; --i) {
		unsigned s = test_random_range(MAX_BLOCKS - 1) + 2;
		for(unsigned j = 0; j < s; ++j)
			(*data)[j] = test_random_u() >> 32;

		memcpy(data_enc, data, sizeof(*data_enc));
		xxtea_encode(*data_enc, s, key);
		if(memcmp(data_enc, data, sizeof(*data_enc)) == 0)
			return -2;

		xxtea_decode(*data_enc, s, key);
		if(memcmp(data_enc, data, sizeof(*data_enc)) != 0)
			return -3;
	}

	free(data_enc);
	free(data);
	return 0;
}

int main(void)
{
	test_parallel("Testing XXTEA cipher proper operation", xxtea_test, NULL, 0);
	// TODO: test randomness properties of the cipher
}
