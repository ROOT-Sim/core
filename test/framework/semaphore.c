/**
 * @file test/framework/semaphore.c
 *
 * @brief Custom minimalistic testing framework: semaphores
 *
 * This module implements multi-platform semaphore support, including a minimalistic
 * spin waiting primitive.
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include "test.h"

#include <assert.h>
#include <stdlib.h>

#if defined(__WINDOWS)

os_semaphore sema_init(unsigned tokens)
{
	return CreateSemaphore(NULL, tokens, MAXLONG, NULL);
}

void sema_remove(os_semaphore sema)
{
	CloseHandle(sema);
}

void sema_wait(os_semaphore sema, unsigned count)
{
	assert(count > 0);
	while(count-- > 0)
		WaitForSingleObject(sema, INFINITE);
}

void sema_signal(os_semaphore sema, unsigned count)
{
	assert(count > 0);
	ReleaseSemaphore(sema, count, NULL);
}

#elif defined(__MACOS)

os_semaphore sema_init(unsigned tokens)
{
	os_semaphore m_sema;
	semaphore_create(mach_task_self(), &m_sema, SYNC_POLICY_FIFO, (int) tokens);
	return m_sema;
}

void sema_remove(os_semaphore sema)
{
	semaphore_destroy(mach_task_self(), sema);
}

void sema_wait(os_semaphore sema, unsigned count)
{
	assert(count > 0);
	while(count-- > 0) {
		semaphore_wait(sema);
	}
}

void sema_signal(os_semaphore sema, unsigned count)
{
	assert(count > 0);
	while(count-- > 0) {
		semaphore_signal(sema);
	}
}

#elif defined(__LINUX)

os_semaphore sema_init(unsigned tokens)
{
	os_semaphore sema = malloc(sizeof(*sema));
	sem_init(sema, 0, tokens);
	return sema;
}

void sema_remove(os_semaphore sema)
{
	sem_destroy(sema);
	free(sema);
}

void sema_wait(os_semaphore sema, unsigned count)
{
	int rc;
	assert(count > 0);
	while (count-- > 0) {
		do {
			rc = sem_wait(sema);
		} while (rc == -1 && errno == EINTR);
	}
}

void sema_signal(os_semaphore sema, unsigned count)
{
	assert(count > 0);
	while (count-- > 0)
	{
		sem_post(sema);
	}
}

#endif
