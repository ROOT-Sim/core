#include <pthread.h>

#include <arch/timer.h>
#include <core/core.h>
#include <datatypes/msg_queue.h>
#include <ftl/ftl.h>

/// This is the id of the GPU thread
static uint gpu_rid = 0;

/// This is the spin barrier for init phase
static volatile unsigned ftl_init_counter;

/// This is the sleep barrier for challenges and used by CPU threads during GPU phases
static pthread_barrier_t xpu_barrier;

/// This is the sleep barrier used by GPU thread during CPU phases
static pthread_barrier_t gpu_barrier;

/// This is a spin barrier to be used for synchronized steps
static volatile unsigned ftl_curr_counter;
static volatile unsigned ftl_spin_barrier = 1;

/// This is the current phase and a spin barrier to be used for synchronized steps
volatile ftl_phase current_phase = INIT;

extern timer_uint gvt_timer;
extern timer_uint gpu_gvt_timer;
extern uint threads_per_block;
extern uint n_blocks;


// TODO: should refactor here
extern void align_host_to_device_parallel(simtime_t gvt);
extern void align_host_to_device(simtime_t gvt);

unsigned dummyphase = 1;


__thread unsigned gvt_rounds = 0;
__thread unsigned free_rounds = 0;


unsigned end_gpu = 0;
unsigned end_cpu = 0;
unsigned both_ended = 0;
simtime_t actual_gvt = 0;

void cpu_ended()
{
	end_cpu = 1;
}
void gpu_ended()
{
	end_gpu = 1;
}

unsigned sim_can_end()
{
	return both_ended;
}

#define FTL_PERIODS 10

void set_gpu_rid(unsigned rid)
{
	gpu_rid = rid;
	ftl_curr_counter = rid + 1;
	ftl_init_counter = rid + 1;

	pthread_barrier_init(&xpu_barrier, NULL, gpu_rid + 1);
	pthread_barrier_init(&gpu_barrier, NULL, 2);
}

void align_device_to_host(int gvt, unsigned n_blocks, unsigned threads_per_block);
void align_device_to_host_parallel(unsigned rid, simtime_t gvt);
void destroy_all_queues(void);

void follow_the_leader(simtime_t current_gvt)
{
	switch(current_phase) {
		case INIT:
			gvt_rounds = 0;
			free_rounds = 0;

			while(!rid && ftl_init_counter > 1);

			if(!rid) {
				printf("cpu has initialized its stuff\n");
				printf("gpu has initialized its stuff\n");
				printf("starting a new challenge\n");
				gvt_timer = timer_new();
				gpu_gvt_timer = gvt_timer;
				__sync_lock_test_and_set(&current_phase, CHALLENGE);
			}

			__sync_fetch_and_add(&ftl_init_counter, -1);
			while(ftl_init_counter); /// all threads should with a challenge, this should be fair

			return;

		case CHALLENGE:
			/// TODO collect samples here

			if((++gvt_rounds % FTL_PERIODS))
				return;

			/// the challenge is going to end
			gvt_rounds = 0;

			/// thread will spin waiting the result of the challenge
			/// this should reduce the impact on performance of the leaders w.r.t. going to sleep

			unsigned waiting = __sync_add_and_fetch(&ftl_curr_counter, -1);

			/// this is execute by the last cpu/gpu thread arriving here
			if(!waiting) {
				ftl_phase new_phase;
				printf("\nthe challenge is completed RID %u\n", rid);
				// printf("the barrier val should be always 0 : %u\n", ftl_curr_counter);


				/// TODO here the code to decide the winner

				/// CPU WINS
				if(dummyphase++ & 1) {
					/// prepare next spin barrier after they wake up
					__sync_lock_test_and_set(&ftl_curr_counter, gpu_rid);
					__sync_lock_test_and_set(&ftl_spin_barrier, 1);
					new_phase = CPU;
				}
				/// GPU WINS
				else {
					new_phase = GPU;
				}

				printf("the winner is %s\n", new_phase == CPU ? "CPU" : "GPU");

				/// realing gvt timers for both cpu and gpu threads
				gvt_timer = timer_new();
				gpu_gvt_timer = gvt_timer;
				__sync_lock_test_and_set(&current_phase, new_phase);
			}

			/// wait here the result of the challenge
			while(current_phase == CHALLENGE);

			/// CPU wins GPU loses
			if(current_phase == CPU && rid == gpu_rid) {
				/// gpu thread go to sleep
				pthread_barrier_wait(&gpu_barrier);
				/// spin wait the init of the next challenge
				while(current_phase != CHALLENGE);
			}

			/// GPU wins CPU loses
			if(current_phase == GPU && rid != gpu_rid) {
				pthread_barrier_wait(&xpu_barrier);

				msg_queue_destroy_all_input_queues();

				unsigned val = __sync_add_and_fetch(&ftl_curr_counter, -1);
				if(val == 1) { /// i am the last one
					/// reinit spin barrier for CPU threads
					if(!__sync_bool_compare_and_swap(&ftl_curr_counter, 1, gpu_rid))
						printf("9 MY CAS FAILED AND THIS SHOULD NEVER HAPPEN 1 vs %u\n",
						    ftl_curr_counter);
					/// unlock all
					__sync_lock_test_and_set(&ftl_spin_barrier, 2);
				}
				while(ftl_spin_barrier == 1);

				align_host_to_device_parallel(actual_gvt); // the gvt here is wrong

				val = __sync_add_and_fetch(&ftl_curr_counter, -1);
				// while(val && ftl_spin_barrier == 2); // all threads -1 will be stucked here
				// if(val) return;
				while(ftl_spin_barrier == 2);

				if(!val)
					printf("resynch with GPU at %'lf\n", actual_gvt);

				while(current_phase != CHALLENGE)
					;

			} // all cpu threads go to sleep

			return;
	}

	/// TODO maybe collect samples also here ??

	if((++free_rounds % FTL_PERIODS))
		return;
	free_rounds = 0;

	unsigned val;
	ftl_phase old_phase;

	switch(current_phase) {
		case CPU:

			val = __sync_add_and_fetch(&ftl_curr_counter, -1);
			while(val && ftl_spin_barrier == 1); // all threads -1 will be stucked here

			if(!val) { /// i am the last one
				/// reinit spin barrier for CPU threads
				if(!__sync_bool_compare_and_swap(&ftl_curr_counter, 0, gpu_rid))
					printf("A MY CAS FAILED AND THIS SHOULD NEVER HAPPEN 0 vs %u\n",
					    ftl_curr_counter);
				/// unlock all
				__sync_lock_test_and_set(&ftl_spin_barrier, 2);
			}

			align_device_to_host_parallel(rid, current_gvt);
			val = __sync_add_and_fetch(&ftl_curr_counter, -1);

			while(val && ftl_spin_barrier == 2); // all threads -1 will be stucked here

			if(val) {
				return;
			}

			printf("aligned memory from CPU SIM to HOST\n");

			/// perform single threaded actions to alkign device to host
			align_device_to_host((int)current_gvt, n_blocks, threads_per_block);

			__attribute__((fallthrough));

		case GPU:

			old_phase = current_phase;
			actual_gvt = current_gvt;

			printf("\n%s phase ended\n", current_phase == CPU ? "CPU" : "GPU");
			printf("\nstarting a new challenge by rid %u curr %u\n", rid, ftl_curr_counter);

			end_cpu = end_cpu || end_gpu;
			end_gpu = end_cpu || end_gpu;
			both_ended = end_cpu && end_gpu;

			/// the check on rid should be useless
			if(old_phase == CPU && rid != gpu_rid) {
				/// wake up gpu thread
				pthread_barrier_wait(&gpu_barrier);
			}

			if(old_phase == GPU && rid == gpu_rid) {
				/// prepare CPU thread spin barrier for align cpu to host
				__sync_lock_test_and_set(&ftl_spin_barrier, 1);
				if(!__sync_bool_compare_and_swap(&ftl_curr_counter, 0, gpu_rid + 1))
					printf("B MY CAS FAILED AND THIS SHOULD NEVER HAPPEN 0 vs %u\n",
					    ftl_curr_counter);

				align_host_to_device(current_gvt);
				printf("aligned memory from DEVICE to HOST\n");

				/// wake up CPU threads
				pthread_barrier_wait(&xpu_barrier);

				/// wait that threads have cleaned their queues/states
				while(ftl_curr_counter);
			}

			/// reinit cpu spin barrier for both cpu/gpu threads
			if(!__sync_bool_compare_and_swap(&ftl_curr_counter, 0, gpu_rid + 1)) {
				printf("C MY CAS FAILED AND THIS SHOULD NEVER HAPPEN 0 vs %u\n", ftl_curr_counter);
			}

			/// unlock any thread spinning here (CPU for aligning device to host or host to device)
			__sync_lock_test_and_set(&ftl_spin_barrier, 0);

			/// reinit gvt timers
			gvt_timer = timer_new();
			gpu_gvt_timer = gvt_timer;

			/// start a new challenge
			printf("\nPrevious phase was %s at %lf ended (%u)\n", old_phase == CPU ? "CPU" : "GPU",
			    current_gvt, rid);
			__sync_lock_test_and_set(&current_phase, CHALLENGE); /// unlock any thread spinning here


			return;

		case INIT:
		case CHALLENGE:
		case END:
			abort();
	}
}
