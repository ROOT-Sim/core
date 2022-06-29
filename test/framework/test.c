/**
 * @file test/framework/test.c
 *
 * @brief Custom minimalistic testing framework
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <stdlib.h>

#include "test.h"
#include "core/core.h"

struct test_unit test_unit = {0};

static struct {
	int passed;
	int xfailed;
	int failed;
	int uxpassed;
} self_test_expected_results = {0};

void self_test(int p, int xf, int f, int uxp)
{
	test_unit.self_test = true;
	self_test_expected_results.passed = p;
	self_test_expected_results.xfailed = xf;
	self_test_expected_results.failed = f;
	self_test_expected_results.uxpassed = uxp;
}

void finish(void)
{
	tear_down_worker_pool();

	int d1 = snprintf(NULL, 0, "PASSED.............: %u / %u\n", test_unit.passed, test_unit.should_pass);
	int d2 = snprintf(NULL, 0, "EXPECTED FAIL......: %u / %u\n", test_unit.xfailed, test_unit.should_fail);
	int d3 = snprintf(NULL, 0, "FAILED.............: %u\n", test_unit.failed);
	int d4 = snprintf(NULL, 0, "UNEXPECTED PASS....: %u\n", test_unit.uxpassed);
	int d = ((d1 > d2 && d1 > d3 && d1 > d4) ? d1 : ((d2 > d3 && d2 > d4) ? d2 : (d3 > d4 ? d3 : d4)));
	printf("%.*s\n", d, "============================================================================");
	printf("PASSED.............: %u / %u\n", test_unit.passed, test_unit.should_pass);
	printf("EXPECTED FAIL......: %u / %u\n", test_unit.xfailed, test_unit.should_fail);
	printf("FAILED.............: %u\n", test_unit.failed);
	printf("UNEXPECTED PASS....: %u\n", test_unit.uxpassed);
	printf("%.*s\n", d, "============================================================================");

	// Self test sanity check
	if(test_unit.self_test) {
		test_unit.ret = -!(test_unit.passed == self_test_expected_results.passed &&
				   test_unit.xfailed == self_test_expected_results.xfailed &&
				   test_unit.failed == self_test_expected_results.failed &&
				   test_unit.uxpassed == self_test_expected_results.uxpassed);

		printf("** SELF TEST SUMMARY **\n");
		printf("Passed: %d - Should be: %d\n", test_unit.passed, self_test_expected_results.passed);
		printf("Expected Fail: %d - Should be: %d\n", test_unit.xfailed, self_test_expected_results.xfailed);
		printf("Failed: %d - Should be: %d\n", test_unit.failed, self_test_expected_results.failed);
		printf("Unexpected Pass: %d - Should be: %d\n", test_unit.uxpassed,
		    self_test_expected_results.uxpassed);
	}

	exit(test_unit.ret);
}

void test_init(unsigned n_th)
{
	test_random_init();
	global_config.n_threads = (n_th);
	spawn_worker_pool(n_th);
}

__attribute__((noreturn)) void fail(void)
{
	fprintf(stderr, "Failing explicitly\n");
	longjmp(test_unit.fail_buffer, 1);
}

void test(char *desc, test_fn test_fn, void *arg)
{
	test_unit.should_pass++;
	printf("%s... ", desc);
	if(test_fn(arg) != 0) {
		test_unit.ret = -1;
		test_unit.failed++;
		printf("FAIL.\n");
		fflush(stdout);
	} else {
		test_unit.passed++;
		printf("passed.\n");
		fflush(stdout);
	}
}


void test_xf(char *desc, test_fn test_fn, void *arg)
{
	test_unit.should_fail++;
	printf("%s... ", desc);
	if(test_fn(arg) == 0) {
		test_unit.ret = -1;
		test_unit.uxpassed++;
		printf("UNEXPECTED PASS.\n");
		fflush(stdout);
	} else {
		test_unit.xfailed++;
		printf("expected fail.\n");
		fflush(stdout);
	}
}

void parallel_test(char *desc, test_fn test_fn, void *args)
{
	int res = 0;
	test_unit.should_pass++;
	printf("%s... ", desc);
	fflush(stdout);
	signal_new_thread_action(test_fn, args);
	for(unsigned i = 0; i < test_unit.n_th; i++)
		res -= test_unit.pool[i].ret;

	if(res != 0) {
		test_unit.ret = -1;
		test_unit.failed++;
		printf("FAIL.\n");
		fflush(stdout);
	} else {
		test_unit.passed++;
		printf("passed.\n");
		fflush(stdout);
	}
}
