/**
 * @file test/semaphore.c
 *
 * @brief Custom minimalistic testing framework: semaphores
 *
 * This module implements multi-platform semaphore support, including a minimalistic
 * spin waiting primitive.
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <stdatomic.h>
#include <stdbool.h>
#include <assert.h>

#include "test.h"


#if defined(_WIN32)

os_semaphore os_create_semaphore(unsigned tokens)
{
    return CreateSemaphore(NULL, tokens, MAXLONG, NULL);
}

void os_destroy_semaphore(os_semaphore sema)
{
    CloseHandle(sema);
}

void os_wait_semaphore(os_semaphore sema)
{
    WaitForSingleObject(sema, INFINITE);
}

void os_signal_semaphore(os_semaphore sema, unsigned count)
{
    assert(count > 0);
    ReleaseSemaphore(sema, count, NULL);
}

#elif defined(__APPLE__) && defined(__MACH__)

os_semaphore os_create_semaphore(unsigned tokens)
{
	os_semaphore m_sema;
	semaphore_create(mach_task_self(), &m_sema, SYNC_POLICY_FIFO, (int)tokens);
	return m_sema;
}

void os_destroy_semaphore(os_semaphore sema)
{
	semaphore_destroy(mach_task_self(), sema);
}

void os_wait_semaphore(os_semaphore sema)
{
	semaphore_wait(sema);
}

void os_signal_semaphore(os_semaphore sema, unsigned count)
{
	assert(count > 0);
	while(count-- > 0) {
		semaphore_signal(sema);
	}
}

#elif defined(__unix__) || defined(__unix)

os_semaphore os_create_semaphore(unsigned tokens)
{
	os_semaphore m_sema = malloc(sizeof(os_semaphore));
	sem_init(&m_sema, 0, tokens);
	return m_sema;
}

void os_destroy_semaphore(os_semaphore sema)
{
	sem_destroy(sema);
	free(sema);
}

void os_wait_semaphore(os_semaphore sema)
{
	int rc;
	do
	{
	    rc = sem_wait(&m_sema);
	}
	while (rc == -1 && errno == EINTR);
}

void os_signal_semaphore(os_semaphore sema, unsigned count)
{
	assert(count > 0);
	while (count-- > 0)
	{
	    sem_post(&m_sema);
	}
}
#else
#error Unsupported operating system
#endif

void sema_init(struct sema_t *sema, unsigned tokens)
{
	sema->count = tokens;
	sema->os_sema = os_create_semaphore(tokens);
}

void sema_remove(struct sema_t *sema)
{
	os_destroy_semaphore(sema->os_sema);
}

static bool sema_trywait(struct sema_t *sema)
{
	int oldCount = atomic_load_explicit(&sema->count, memory_order_relaxed);
	return (oldCount > 0 && atomic_compare_exchange_strong_explicit(&sema->count, &oldCount, oldCount - 1,
									memory_order_acquire, memory_order_acquire));
}

static void sema_spin_and_wait(struct sema_t *sema)
{
	int oldCount;
	unsigned spin = 10000;
	while(spin--) {
		oldCount = atomic_load_explicit(&sema->count, memory_order_relaxed);
		if((oldCount > 0)
		   &&  atomic_compare_exchange_strong_explicit(&sema->count, &oldCount, oldCount - 1,
							       memory_order_acquire, memory_order_acquire)) {
			return;
		}
		atomic_signal_fence(memory_order_acquire);
	}
	oldCount = atomic_fetch_sub_explicit(&sema->count, 1, memory_order_acquire);
	if(oldCount <= 0) {
		os_wait_semaphore(sema->os_sema);
	}
}

void sema_wait(struct sema_t *sema, int count)
{
	while(count--) {
		if(!sema_trywait(sema)) {
			sema_spin_and_wait(sema);
		}
	}
}

void sema_signal(struct sema_t *sema, int count)
{
	assert(count > 0);
	int oldCount = atomic_fetch_add_explicit(&sema->count, count, memory_order_release);
	int toRelease = -oldCount < count ? -oldCount : count;
	if(toRelease > 0) {
		os_signal_semaphore(sema->os_sema, toRelease);
	}
}
