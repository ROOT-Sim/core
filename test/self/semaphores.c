#include <test.h>
#include <core/core.h>

#define N_PROD 1
#define N_CONS 2
#define REP_COUNT 1000
#define RAND_DELAY_MAX 100000

#define N_THREAD (N_CONS + N_PROD)

_Static_assert(N_PROD > 0, "Must have a positive number of producers");
_Static_assert(N_CONS > 0, "Must have a positive number of consumers");
_Static_assert(N_CONS % N_PROD == 0, "The number of consumers must be a multiple of producers");

_Atomic int prod_exec = 0;
_Atomic int cons_exec = 0;

struct sema_t producer;
struct sema_t consumer;

thr_id_t tids[N_THREAD];
unsigned seed[N_THREAD];

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
	unsigned long tid = (unsigned long)id;
	int r = rand_r(&seed[tid]) % RAND_DELAY_MAX;
	for(int i = 0; i < r; i++);

	if(tid < N_PROD) {
		sema_signal(&producer, N_CONS/N_PROD);
		prod_exec++;
	} else {
		sema_wait(&producer, 1);
		cons_exec++;
	}
	return 0;
}

thrd_ret_t phase2(void *id)
{
	unsigned long tid = (unsigned long)id;
	int r = rand_r(&seed[tid]) % RAND_DELAY_MAX;
	for(int i = 0; i < r; i++);

	if(tid < N_PROD) {
		sema_wait(&consumer, N_CONS/N_PROD);
		prod_exec++;
	} else {
		sema_signal(&consumer, 1);
		cons_exec++;
	}
	return 0;
}



#define DESC_LEN 128
int main(void)
{
	char desc[DESC_LEN];

	init(0); // Not using framework thread pool support here, to decouple testing

	srand(time(NULL));
	for(int i = 0; i < N_THREAD; i++)
		seed[i] = rand(); // We don't really care about stochastic properties here!

	sema_init(&producer, 0);
	sema_init(&consumer, 0);

	for(int i = 1; i < REP_COUNT+1; i++) {
		snprintf(desc, DESC_LEN, "Testing producer phase %d", i);

		for(unsigned long j = 0; i < N_THREAD; i++)
			sem_test_thread_start(&tids[j], phase1, (void *)j);
		for(unsigned long j = 0; i < N_THREAD; i++)
			sem_test_thread_wait(tids[j], NULL);

		printf("%d.1: prod %d - cons %d", i, prod_exec, cons_exec);

		assert(prod_exec == 2 * i * N_PROD - N_PROD);
		assert(cons_exec == 2 * i * N_CONS - N_CONS);

		snprintf(desc, DESC_LEN, "Testing consumer phase %d", i);

		for(unsigned long j = 0; i < N_THREAD; i++)
			sem_test_thread_start(&tids[j], phase2, (void *)j);
		for(unsigned long j = 0; i < N_THREAD; i++)
			sem_test_thread_wait(tids[j], NULL);

		printf("%d.2: prod %d - cons %d", i, prod_exec, cons_exec);

		assert(prod_exec == 2 * i * N_PROD);
		assert(cons_exec == 2 * i * N_CONS);
	}

	finish();
}
