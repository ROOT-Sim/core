/**
 * @file arch/threads.h
 *
 * @brief Portable C11 Thread Implementation
 *
 * This is a C11-mostly-compliant version of threading support targeting both
 * POSIX and Windows systems.
 * It is heavily based on Tiny C Threads, which sources have been slightly modified
 * to account for ROOT-Sim's code base, and to slightly improve the performance
 * of some primitive.
 *
 * @copyright
 * Copyright (c) 2012 Marcus Geelnard
 * Copyright (c) 2013-2016 Evan Nemerson
 * https://tinycthread.github.io/
 * Copyright (c) 2020 HPDCS Group
 * https://rootsim.github.io/core
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *   1. The origin of this software must not be misrepresented; you must not
 *   claim that you wrote the original software. If you use this software
 *   in a product, an acknowledgment in the product documentation would be
 *   appreciated but is not required.
 *   2. Altered source versions must be plainly marked as such, and must not be
 *   misrepresented as being the original software.
 *   3. This notice may not be removed or altered from any source
 *   distribution.
 */
#pragma once

#include <arch/platform.h>

#include <time.h>

/* Platform specific includes */
#if defined(__POSIX)
#include <pthread.h>
#elif defined(__WINDOWS)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#define __UNDEF_LEAN_AND_MEAN
#endif
#include <windows.h>
#ifdef __UNDEF_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#undef __UNDEF_LEAN_AND_MEAN
#endif
#endif

/* If TIME_UTC is missing, provide it and provide a wrapper for
   timespec_get. */
#ifndef TIME_UTC
#define TIME_UTC 1
#define _TTHREAD_EMULATE_TIMESPEC_GET_

#if defined(__WINDOWS)
struct _tthread_timespec {
  time_t tv_sec;
  long   tv_nsec;
};
#define timespec _tthread_timespec
#endif

int _tthread_timespec_get(struct timespec *ts, int base);

#define timespec_get _tthread_timespec_get
#endif

/* Macros */
#if defined(__WINDOWS)
#define TSS_DTOR_ITERATIONS (4)
#else
#define TSS_DTOR_ITERATIONS PTHREAD_DESTRUCTOR_ITERATIONS
#endif

/* Function return values */
#define thrd_error    0 /**< The requested operation failed */
#define thrd_success  1 /**< The requested operation succeeded */
#define thrd_timedout 2 /**< The time specified in the call was reached without acquiring the requested resource */
#define thrd_busy     3 /**< The requested operation failed because a tesource requested by a test and return function is already in use */
#define thrd_nomem    4 /**< The requested operation failed because it was unable to allocate memory */

/* Mutex types */
#define mtx_plain     0
#define mtx_timed     1
#define mtx_recursive 2

/* Mutex */
#if defined(__WINDOWS)
typedef struct {
  union {
	CRITICAL_SECTION cs;      /* Critical section handle (used for non-timed mutexes) */
	HANDLE mut;               /* Mutex handle (used for timed mutex) */
  } mHandle;                  /* Mutex handle */
  int mAlreadyLocked;         /* TRUE if the mutex is already locked */
  int mRecursive;             /* TRUE if the mutex is recursive */
  int mTimed;                 /* TRUE if the mutex is timed */
} mtx_t;
#else
typedef pthread_mutex_t mtx_t;
#endif

/** Create a mutex object.
* @param mtx A mutex object.
* @param type Bit-mask that must have one of the following six values:
*   @li @c mtx_plain for a simple non-recursive mutex
*   @li @c mtx_timed for a non-recursive mutex that supports timeout
*   @li @c mtx_plain | @c mtx_recursive (same as @c mtx_plain, but recursive)
*   @li @c mtx_timed | @c mtx_recursive (same as @c mtx_timed, but recursive)
* @return @ref thrd_success on success, or @ref thrd_error if the request could
* not be honored.
*/
int mtx_init(mtx_t *mtx, int type);

/** Release any resources used by the given mutex.
* @param mtx A mutex object.
*/
void mtx_destroy(mtx_t *mtx);

/** Lock the given mutex.
* Blocks until the given mutex can be locked. If the mutex is non-recursive, and
* the calling thread already has a lock on the mutex, this call will block
* forever.
* @param mtx A mutex object.
* @return @ref thrd_success on success, or @ref thrd_error if the request could
* not be honored.
*/
int mtx_lock(mtx_t *mtx);

/** Lock the given mutex, or block until a specific point in time.
* Blocks until either the given mutex can be locked, or the specified TIME_UTC
* based time.
* @param mtx A mutex object.
* @param ts A UTC based calendar time
* @return @ref The mtx_timedlock function returns thrd_success on success, or
* thrd_timedout if the time specified was reached without acquiring the
* requested resource, or thrd_error if the request could not be honored.
*/
int mtx_timedlock(mtx_t *mtx, const struct timespec *ts);

/** Try to lock the given mutex.
* The specified mutex shall support either test and return or timeout. If the
* mutex is already locked, the function returns without blocking.
* @param mtx A mutex object.
* @return @ref thrd_success on success, or @ref thrd_busy if the resource
* requested is already in use, or @ref thrd_error if the request could not be
* honored.
*/
int mtx_trylock(mtx_t *mtx);

/** Unlock the given mutex.
* @param mtx A mutex object.
* @return @ref thrd_success on success, or @ref thrd_error if the request could
* not be honored.
*/
int mtx_unlock(mtx_t *mtx);

/* Condition variable */
#if defined(__WINDOWS)
typedef struct {
  HANDLE mEvents[2];                  /* Signal and broadcast event HANDLEs. */
  unsigned int mWaitersCount;         /* Count of the number of waiters. */
  CRITICAL_SECTION mWaitersCountLock; /* Serialize access to mWaitersCount. */
} cnd_t;
#else
typedef pthread_cond_t cnd_t;
#endif

/** Create a condition variable object.
* @param cond A condition variable object.
* @return @ref thrd_success on success, or @ref thrd_error if the request could
* not be honored.
*/
int cnd_init(cnd_t *cond);

/** Release any resources used by the given condition variable.
* @param cond A condition variable object.
*/
void cnd_destroy(cnd_t *cond);

/** Signal a condition variable.
* Unblocks one of the threads that are blocked on the given condition variable
* at the time of the call. If no threads are blocked on the condition variable
* at the time of the call, the function does nothing and return success.
* @param cond A condition variable object.
* @return @ref thrd_success on success, or @ref thrd_error if the request could
* not be honored.
*/
int cnd_signal(cnd_t *cond);

/** Broadcast a condition variable.
* Unblocks all of the threads that are blocked on the given condition variable
* at the time of the call. If no threads are blocked on the condition variable
* at the time of the call, the function does nothing and return success.
* @param cond A condition variable object.
* @return @ref thrd_success on success, or @ref thrd_error if the request could
* not be honored.
*/
int cnd_broadcast(cnd_t *cond);

/** Wait for a condition variable to become signaled.
* The function atomically unlocks the given mutex and endeavors to block until
* the given condition variable is signaled by a call to cnd_signal or to
* cnd_broadcast. When the calling thread becomes unblocked it locks the mutex
* before it returns.
* @param cond A condition variable object.
* @param mtx A mutex object.
* @return @ref thrd_success on success, or @ref thrd_error if the request could
* not be honored.
*/
int cnd_wait(cnd_t *cond, mtx_t *mtx);

/** Wait for a condition variable to become signaled.
* The function atomically unlocks the given mutex and endeavors to block until
* the given condition variable is signaled by a call to cnd_signal or to
* cnd_broadcast, or until after the specified time. When the calling thread
* becomes unblocked it locks the mutex before it returns.
* @param cond A condition variable object.
* @param mtx A mutex object.
* @param xt A point in time at which the request will time out (absolute time).
* @return @ref thrd_success upon success, or @ref thrd_timeout if the time
* specified in the call was reached without acquiring the requested resource, or
* @ref thrd_error if the request could not be honored.
*/
int cnd_timedwait(cnd_t *cond, mtx_t *mtx, const struct timespec *ts);

/* Thread */
#if defined(__WINDOWS)
typedef HANDLE thrd_t;
#else
typedef pthread_t thrd_t;
#endif

/** Thread start function.
* Any thread that is started with the @ref thrd_create() function must be
* started through a function of this type.
* @param arg The thread argument (the @c arg argument of the corresponding
*        @ref thrd_create() call).
* @return The thread return value, which can be obtained by another thread
* by using the @ref thrd_join() function.
*/
typedef int (*thrd_start_t)(void *arg);

/** Create a new thread.
* @param thr Identifier of the newly created thread.
* @param func A function pointer to the function that will be executed in
*        the new thread.
* @param arg An argument to the thread function.
* @return @ref thrd_success on success, or @ref thrd_nomem if no memory could
* be allocated for the thread requested, or @ref thrd_error if the request
* could not be honored.
* @note A thread’s identifier may be reused for a different thread once the
* original thread has exited and either been detached or joined to another
* thread.
*/
int thrd_create(thrd_t *thr, thrd_start_t func, void *arg);

/** Identify the calling thread.
* @return The identifier of the calling thread.
*/
thrd_t thrd_current(void);

/** Dispose of any resources allocated to the thread when that thread exits.
 * @return thrd_success, or thrd_error on error
*/
int thrd_detach(thrd_t thr);

/** Compare two thread identifiers.
* The function determines if two thread identifiers refer to the same thread.
* @return Zero if the two thread identifiers refer to different threads.
* Otherwise a nonzero value is returned.
*/
int thrd_equal(thrd_t thr0, thrd_t thr1);

/** Terminate execution of the calling thread.
* @param res Result code of the calling thread.
*/
TTHREAD_NORETURN void thrd_exit(int res);

/** Wait for a thread to terminate.
* The function joins the given thread with the current thread by blocking
* until the other thread has terminated.
* @param thr The thread to join with.
* @param res If this pointer is not NULL, the function will store the result
*        code of the given thread in the integer pointed to by @c res.
* @return @ref thrd_success on success, or @ref thrd_error if the request could
* not be honored.
*/
int thrd_join(thrd_t thr, int *res);

/** Put the calling thread to sleep.
* Suspend execution of the calling thread.
* @param duration  Interval to sleep for
* @param remaining If non-NULL, this parameter will hold the remaining
*                  time until time_point upon return. This will
*                  typically be zero, but if the thread was woken up
*                  by a signal that is not ignored before duration was
*                  reached @c remaining will hold a positive time.
* @return 0 (zero) on successful sleep, -1 if an interrupt occurred,
*         or a negative value if the operation fails.
*/
int thrd_sleep(const struct timespec *duration, struct timespec *remaining);

/** Yield execution to another thread.
* Permit other threads to run, even if the current thread would ordinarily
* continue to run.
*/
void thrd_yield(void);

/* Thread local storage */
#if defined(__WINDOWS)
typedef DWORD tss_t;
#else
typedef pthread_key_t tss_t;
#endif

/** Destructor function for a thread-specific storage.
* @param val The value of the destructed thread-specific storage.
*/
typedef void (*tss_dtor_t)(void *val);

/** Create a thread-specific storage.
* @param key The unique key identifier that will be set if the function is
*        successful.
* @param dtor Destructor function. This can be NULL.
* @return @ref thrd_success on success, or @ref thrd_error if the request could
* not be honored.
* @note On Windows, the @c dtor will definitely be called when
* appropriate for threads created with @ref thrd_create.  It will be
* called for other threads in most cases, the possible exception being
* for DLLs loaded with LoadLibraryEx.  In order to be certain, you
* should use @ref thrd_create whenever possible.
*/
int tss_create(tss_t *key, tss_dtor_t dtor);

/** Delete a thread-specific storage.
* The function releases any resources used by the given thread-specific
* storage.
* @param key The key that shall be deleted.
*/
void tss_delete(tss_t key);

/** Get the value for a thread-specific storage.
* @param key The thread-specific storage identifier.
* @return The value for the current thread held in the given thread-specific
* storage.
*/
void *tss_get(tss_t key);

/** Set the value for a thread-specific storage.
* @param key The thread-specific storage identifier.
* @param val The value of the thread-specific storage to set for the current
*        thread.
* @return @ref thrd_success on success, or @ref thrd_error if the request could
* not be honored.
*/
int tss_set(tss_t key, void *val);

#if defined(__WINDOWS)
typedef struct {
	LONG volatile status;
	CRITICAL_SECTION lock;
} once_flag;
#define ONCE_FLAG_INIT {0,}
#else
#define once_flag pthread_once_t
#define ONCE_FLAG_INIT PTHREAD_ONCE_INIT
#endif

/** Invoke a callback exactly once
 * @param flag Flag used to ensure the callback is invoked exactly
 *        once.
 * @param func Callback to invoke.
 */
#if defined(__WINDOWS)
void call_once(once_flag *flag, void (*func)(void));
#else
#define call_once(flag, func) pthread_once(flag,func)
#endif
