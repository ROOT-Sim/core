/**
* @file test/self-tests/semaphores.c
*
* @brief Test: Test for the thread synchronization primitives used in tests
*
* A simple test to verify synchronization primitives used to synchronize parallel tests
*
* SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
* SPDX-License-Identifier: GPL-3.0-only
 */
#include "test.h"
#include "core/core.h"

#define N_PROD 1
#define N_CONS 1
#define REP_COUNT 1000
#define RAND_DELAY_MAX 10000

#define N_THREAD (N_CONS + N_PROD)

_Static_assert(N_PROD > 0, "Must have a positive number of producers");
_Static_assert(N_CONS > 0, "Must have a positive number of consumers");
_Static_assert(N_CONS % N_PROD == 0, "The number of consumers must be a multiple of producers");

_Atomic int prod_exec_1 = 0;
_Atomic int cons_exec_1 = 0;
_Atomic int prod_exec_2 = 0;
_Atomic int cons_exec_2 = 0;

os_semaphore producer;
os_semaphore consumer;

thr_id_t tids[N_THREAD];

// Using yet another thread implementation to decouple testing
// semaphores from testing thread pools
#if defined(__unix__) || defined(__unix) || defined(__APPLE__) && defined(__MACH__)
test_ret_t sem_test_thread_start(thr_id_t *thr_p, thr_run_fnc t_fnc, void *t_fnc_arg)
{
	return (pthread_create(thr_p, NULL, t_fnc, t_fnc_arg) != 0);
}

test_ret_t sem_test_thread_wait(thr_id_t thr, thrd_ret_t *ret)
{
	return (pthread_join(thr, ret) != 0);
}
#elif defined(_WIN32)
test_ret_t sem_test_thread_start(thr_id_t *thr_p, thr_run_fnc t_fnc, void *t_fnc_arg)
{
	*thr_p = CreateThread(NULL, 0, t_fnc, t_fnc_arg, 0, NULL);
	return (*thr_p == NULL);
}

test_ret_t sem_test_thread_wait(thr_id_t thr, thrd_ret_t *ret)
{
	if(WaitForSingleObject(thr, INFINITE) == WAIT_FAILED)
		return 1;

	if(ret)
		return (GetExitCodeThread(thr, ret) == 0);

	return 0;
}
#else
#error Unsupported operating system
#endif

thrd_ret_t phase1(void *id)
{
	uintptr_t tid = (uintptr_t)id;
	uint64_t r = test_random_range(RAND_DELAY_MAX);
	for(uint64_t i = 0; i < r; i++); // FIXME: this would get for sure optimized away

	if(tid < N_PROD) {
		sema_signal(producer, N_CONS/N_PROD);
		prod_exec_1++;
	} else {
		sema_wait(producer, 1);
		cons_exec_1++;
	}
	return 0;
}

thrd_ret_t phase2(void *id)
{
	uintptr_t tid = (uintptr_t)id;
	uint64_t r = test_random_range(RAND_DELAY_MAX);
	for(uint64_t i = 0; i < r; i++); // FIXME: this would get for sure optimized away

	if(tid < N_PROD) {
		sema_wait(consumer, N_CONS/N_PROD);
		prod_exec_2++;
	} else {
		sema_signal(consumer, 1);
		cons_exec_2++;
	}

	fflush(stdout);
	return 0;
}

test_ret_t test_semaphores(__unused void *_)
{
	producer = sema_init(0);
	consumer = sema_init(0);

	for(int i = 1; i < REP_COUNT+1; i++) {
		for(uintptr_t j = 0; j < N_THREAD; j++)
			sem_test_thread_start(&tids[j], phase1, (void *)j);
		for(uintptr_t j = 0; j < N_THREAD; j++)
			sem_test_thread_wait(tids[j], NULL);

		test_assert(prod_exec_1 == N_PROD * i);
		test_assert(cons_exec_1 == N_CONS * i);

		for(uintptr_t j = 0; j < N_THREAD; j++)
			sem_test_thread_start(&tids[j], phase2, (void *)(uintptr_t)j);
		for(uintptr_t j = 0; j < N_THREAD; j++)
			sem_test_thread_wait(tids[j], NULL);

		test_assert(prod_exec_2 == N_PROD * i);
		test_assert(cons_exec_2 == N_CONS * i);
	}
	check_passed_asserts();
}
