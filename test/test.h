/**
* @file test/test.h
*
* @brief Custom minimalistic testing framework
*
* SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
* SPDX-License-Identifier: GPL-3.0-only
*/
#pragma once

#include <stdio.h>
#include <setjmp.h>
#include <arch/thread.h>

static struct {
	unsigned n_th;
	thr_id_t *thrs;
	jmp_buf fail_buffer;
	int ret;
	unsigned total;
	unsigned passed;
	unsigned failed;
	unsigned xfailed;
	unsigned uxpassed;
	unsigned should_pass;
	unsigned should_fail;
} test_unit = {0};


#define assert(condition)                                                                                              \
        do {                                                                                                           \
                if(!(condition)) {                                                                                     \
                        fprintf(stderr, "assertion failed: " #condition " at %s:%d\n", __FILE__, __LINE__);            \
                        test_unit.ret = -1;                                                                            \
                }                                                                                                      \
        } while(0)

#define init()                                                                                                         \
        do {                                                                                                           \
                if(setjmp(test_unit.fail_buffer)) {                                                                    \
                        test_unit.ret = -1;                                                                            \
                        finish();                                                                                      \
                }                                                                                                      \
        } while(0)

#define check_passed_asserts()                                                                                         \
        do {                                                                                                           \
                int ret = test_unit.ret;                                                                               \
                test_unit.ret = 0;                                                                                     \
                return ret;                                                                                            \
        } while(0)

#define finish()                                                                                                       \
        do {                                                                                                           \
                int d1 = snprintf(NULL, 0, "PASSED.............: %u / %u\n", test_unit.passed, test_unit.should_pass); \
                int d2 = snprintf(NULL, 0, "EXPECTED FAIL......: %u / %u\n", test_unit.xfailed, test_unit.should_fail);\
                int d3 = snprintf(NULL, 0, "FAILED.............: %u\n", test_unit.failed);                             \
                int d4 = snprintf(NULL, 0, "UNEXPECTED PASS....: %u\n", test_unit.uxpassed);                           \
                int d = ((d1 > d2 && d1 > d3 && d1 > d4) ? d1 : ((d2 > d3 && d2 > d4) ? d2 : (d3 > d4 ? d3 : d4)));    \
                printf("%.*s\n", d, "============================================================================");   \
                printf("PASSED.............: %u / %u\n", test_unit.passed, test_unit.should_pass);                     \
                printf("EXPECTED FAIL......: %u / %u\n", test_unit.xfailed, test_unit.should_fail);                    \
                printf("FAILED.............: %u\n", test_unit.failed);                                                 \
                printf("UNEXPECTED PASS....: %u\n", test_unit.uxpassed);                                               \
                printf("%.*s\n", d, "============================================================================");   \
                return test_unit.ret;                                                                                  \
        } while(0)

#define fail()                                                                                                         \
        do {                                                                                                           \
                fprintf(stderr, "Failing explicitly\n");                                                               \
                longjmp(test_unit.fail_buffer, 1);                                                                     \
        } while(0)

#define test(desc, function, ...)                                                                                      \
        do {                                                                                                           \
                test_unit.should_pass++;                                                                               \
                printf(desc "... ");                                                                                   \
                if(function(__VA_ARGS__) != 0) {                                                                       \
                        test_unit.ret = -1;                                                                            \
                        test_unit.failed++;                                                                            \
                        printf("FAIL.\n");                                                                             \
                        fflush(stdout);                                                                                \
                } else {                                                                                               \
                        test_unit.passed++;                                                                            \
                        printf("passed.\n");                                                                           \
                        fflush(stdout);                                                                                \
                }                                                                                                      \
        } while(0)

#define test_xf(desc, function, ...)                                                                                   \
        do {                                                                                                           \
                test_unit.should_fail++;                                                                               \
                printf(desc "... ");                                                                                   \
                if(function(__VA_ARGS__) == 0) {                                                                       \
                        test_unit.ret = -1;                                                                            \
                        test_unit.uxpassed++;                                                                          \
                        printf("UNEXPECTED PASS.\n");                                                                  \
                } else {                                                                                               \
                        test_unit.xfailed++;                                                                           \
                        printf("expected fail.\n");                                                                    \
                }                                                                                                      \
        } while(0)


#define parallel_test(desc, n_th, function)                                                                            \
        do {                                                                                                           \
                unsigned i = n_th;                                                                                     \
                unsigned failed_thr = 0;                                                                               \
                test_unit.should_pass++;                                                                               \
                printf(desc "... ");                                                                                   \
                while(i--) {                                                                                           \
                        rid = i;                                                                                       \
                        if(thread_start(&thrs[i], function, &rid)) {                                                   \
                                fprintf(stderr, "Unable to create thread %u/%d", i, N_THREADS);                        \
                                fail();                                                                                \
                        }                                                                                              \
                }                                                                                                      \
                i = n_th;                                                                                              \
                while(i--) {                                                                                           \
                        thr_ret_t ret;                                                                                 \
                        thread_wait(thrs[i], &ret);                                                                    \
                        if(ret != 0)                                                                                   \
                                failed_thr++;                                                                          \
                }                                                                                                      \
                if(failed_thr == 0) {                                                                                  \
                        test_unit.ret = -1;                                                                            \
                        test_unit.passed++;                                                                            \
                        printf("passed.\n");                                                                           \
                } else {                                                                                               \
                        test_unit.failed++;                                                                            \
                        printf("FAIL.\n");                                                                             \
                }                                                                                                      \
        } while(0)
