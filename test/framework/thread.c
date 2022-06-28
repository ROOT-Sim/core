/**
 * @file test/framework/threads.c
 *
 * @brief Custom minimalistic testing framework: multiplatform thread pool support
 *
 * This module implements a minimalistic thread pool support, which allows to spawn
 * multiple threads at the beginning of a concurrency test.
 * These threads then wait for functions to be executed, as single unitary tests.
 * The state of the thread is therefore kept across the whole test unit.
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <string.h>
#include <test.h>

#include <core/core.h>

static test_fn next_test;
static void *next_args;
static os_semaphore work;
static os_semaphore ready;
static os_semaphore all_done;

#if defined(__POSIX)

test_ret_t test_thread_start(thr_id_t *thr_p, thr_run_fnc t_fnc, void *t_fnc_arg)
{
	return (pthread_create(thr_p, NULL, t_fnc, t_fnc_arg) != 0);
}

test_ret_t test_thread_wait(thr_id_t thr, thrd_ret_t *ret)
{
	return (pthread_join(thr, ret) != 0);
}

#elif defined(__WINDOWS)

test_ret_t test_thread_start(thr_id_t *thr_p, thr_run_fnc t_fnc, void *t_fnc_arg)
{
	*thr_p = CreateThread(NULL, 0, t_fnc, t_fnc_arg, 0, NULL);
	return (*thr_p == NULL);
}

test_ret_t test_thread_wait(thr_id_t thr, thrd_ret_t *ret)
{
	if(WaitForSingleObject(thr, INFINITE) == WAIT_FAILED)
		return 1;

	if(ret)
		return (GetExitCodeThread(thr, ret) == 0);

	return 0;
}

#endif

thrd_ret_t test_worker(void *args)
{
	struct worker *worker = (struct worker *)args;

	rid = (unsigned)worker->rid;

	test_random_init();

	while(true) {
		sema_wait(work, 1);
		if(next_test != NULL)
			worker->ret -= next_test(next_args);
		sema_signal(ready, 1);
		if(next_test == NULL)
			break;
	}
	sema_signal(ready, 1);
	return THREAD_RET_SUCCESS;
}

void spawn_worker_pool(unsigned n_th)
{
	if(n_th == 0)
		return;

	test_unit.n_th = n_th;
	rid = (rid_t)-1;
	work = sema_init(0);
	ready = sema_init(0);
	all_done = sema_init(0);

	test_unit.pool = malloc(sizeof(*test_unit.pool) * test_unit.n_th);
	memset(test_unit.pool, 0, sizeof(*test_unit.pool) * test_unit.n_th);
	unsigned i = test_unit.n_th - 1;
	do {
		test_unit.pool[i].rid = i;
		test_thread_start(&test_unit.pool[i].tid, test_worker, &test_unit.pool[i]);
	} while(i-- > 0);
}

int signal_new_thread_action(test_fn fn, void *args)
{
	if(test_unit.n_th == 0) {
		fprintf(stderr, "Error: invoking a thread action, but the thread pool was initialized with 0 threads\n");
		abort();
	}

  	int ret = 0;
	next_test = fn;
	next_args = args;
	sema_signal(work, (int)test_unit.n_th);
	sema_wait(ready, (int)test_unit.n_th);
	for(unsigned i = 0; i < test_unit.n_th; i++)
		ret -= test_unit.pool[i].ret;
	sema_signal(all_done, (int)test_unit.n_th);
	return ret;
}

void tear_down_worker_pool(void)
{
	if(test_unit.n_th == 0)
		return;

	signal_new_thread_action(NULL, NULL);
	for(unsigned i = 0; i < test_unit.n_th; i++)
		test_thread_wait(test_unit.pool[i].tid, NULL);

	sema_remove(work);
	sema_remove(ready);
	sema_remove(all_done);

	free(test_unit.pool);
}
