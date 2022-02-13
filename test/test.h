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
#include <assert.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
typedef HANDLE os_semaphore;
#elif defined(__APPLE__) && defined(__MACH__)
#include <mach/mach.h>
typedef semaphore_t os_semaphore;
#elif defined(__unix__) || defined(__unix)
#include <errno.h>
#include <semaphore.h>
typedef sem_t * os_semaphore;
#else
#error Unsupported operating system
#endif



struct sema_t {
	_Atomic int count;
	os_semaphore os_sema;
};

#if defined(__unix__) || defined(__unix) || defined(__APPLE__) && defined(__MACH__)
#define _GNU_SOURCE
#include <pthread.h>

#define THREAD_CALL_CONV
typedef void * thrd_ret_t;
typedef pthread_t thr_id_t;

#elif defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN

#define THREAD_CALL_CONV WINAPI
typedef DWORD thrd_ret_t;
typedef HANDLE thr_id_t;

#define THREAD_RET_FAILURE (1)
#define THREAD_RET_SUCCESS (0)

#else
#error Unsupported operating system
#endif

typedef thrd_ret_t(*thr_run_fnc)(void *);

struct worker {
	thr_id_t tid;
	unsigned rid;
	int ret;
};

struct test_unit {
	unsigned n_th;
	struct worker *pool;
	jmp_buf fail_buffer;
	int ret;
	unsigned total;
	unsigned passed;
	unsigned failed;
	unsigned xfailed;
	unsigned uxpassed;
	unsigned should_pass;
	unsigned should_fail;
};

typedef unsigned test_ret_t;
typedef test_ret_t(*test_fn)(void *);

extern test_ret_t test_thread_start(thr_id_t *thr_p, thr_run_fnc t_fnc, void *t_fnc_arg);
extern test_ret_t test_thread_wait(thr_id_t thr, thrd_ret_t *ret);

extern struct test_unit test_unit;

extern void spawn_worker_pool(unsigned n_th);
int signal_new_thread_action(test_fn fn, void *args);
void tear_down_worker_pool(void);

extern void sema_init(struct sema_t *sema, unsigned tokens);
extern void sema_remove(struct sema_t *sema);
extern void sema_wait(struct sema_t *sema, int count);
extern void sema_signal(struct sema_t *sema, int count);


/****+ API TO BE USED IN TESTS ARE DECLARED BELOW THIS LINE ******/

#define test_assert(condition)                                                                                              \
        do {                                                                                                           \
                if(!(condition)) {                                                                                     \
                        fprintf(stderr, "assertion failed: " #condition " at %s:%d\n", __FILE__, __LINE__);            \
                        test_unit.ret = -1;                                                                            \
                }                                                                                                      \
        } while(0)

#define check_passed_asserts()                                                                                         \
        do {                                                                                                           \
                int ret = test_unit.ret;                                                                               \
                test_unit.ret = 0;                                                                                     \
                return ret;                                                                                            \
        } while(0)


extern void finish(void);
extern void init(unsigned n_th);
extern void fail(void);
extern void test(char *desc, test_fn test_fn, void *arg);
extern void test_xf(char *desc, test_fn test_fn, void *arg);
extern void parallel_test(char *desc, test_fn test_fn, void *args);
