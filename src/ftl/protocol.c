#include <pthread.h>

#include <arch/timer.h>
#include <core/core.h>
#include <datatypes/msg_queue.h>
#include <ftl/ftl.h>
#include <log/log.h>

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
static volatile unsigned ftl_spin_barrier = 0;

/// This is the current phase and a spin barrier to be used for synchronized steps
volatile ftl_phase current_phase = INIT;

extern timer_uint gvt_timer;
extern timer_uint gpu_gvt_timer;
extern uint threads_per_block;
extern uint n_blocks;


// TODO: should refactor here
extern void align_host_to_device_parallel(simtime_t gvt);
extern void align_host_to_device(simtime_t gvt);

unsigned dummyphase = 0;


__thread unsigned gvt_rounds = 0;
__thread unsigned free_rounds = 0;


#define GPU_THREAD 0
#define CPU_THREAD 1
#define CPU_MAIN_THREAD 2

#define WHO_AM_I  ( (rid != gpu_rid) + (rid == 0))


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
#define USE_DUMMY_CMP_SPEED 1


#define ALL_THREADS_CNT (gpu_rid+1)
#define GPU_THREADS_CNT (1)
#define CPU_THREADS_CNT (gpu_rid)


void set_gpu_rid(unsigned my_rid)
{
	gpu_rid = my_rid;
	ftl_curr_counter = my_rid + 1;
	ftl_init_counter = my_rid + 1;

	pthread_barrier_init(&xpu_barrier, NULL, ALL_THREADS_CNT);
	pthread_barrier_init(&gpu_barrier, NULL, GPU_THREADS_CNT+1);
    __sync_synchronize();
}


static inline void reset_ftl_barrier(unsigned expected_threads, unsigned next_round_threads, unsigned expected_barrier, unsigned next_barrier_value){
    if(!__sync_bool_compare_and_swap(&ftl_curr_counter, expected_threads, next_round_threads)){
        logger(LOG_WARN, "%u MY CAS FAILED AND THIS SHOULD NEVER HAPPEN expected %u vs %u", expected_threads, ftl_curr_counter);
    }
    if(!__sync_bool_compare_and_swap(&ftl_spin_barrier, expected_barrier, next_barrier_value)){
        logger(LOG_WARN, "%u MY CAS FAILED AND THIS SHOULD NEVER HAPPEN expected %u vs %u", expected_barrier, ftl_spin_barrier);        
    }
}


static inline int ftl_transition(enum ftl_phase curr, enum ftl_phase next){
    int res = __sync_bool_compare_and_swap(&current_phase, curr, next); 
    if(!res){
        logger(LOG_WARN, "%u MY CAS FAILED AND THIS SHOULD NEVER HAPPEN expected %u vs %u", curr, current_phase);                
    }
    return res;              
}

unsigned challenge_count = 0;


static timer_uint wall_clock_timer;
	
extern void align_device_to_host(int gvt, unsigned n_blocks, unsigned threads_per_block);
extern void align_device_to_host_parallel_events(unsigned rid, simtime_t gvt);
extern void align_device_to_host_parallel_states(unsigned rid, simtime_t gvt);
extern void destroy_all_queues(void);
extern unsigned get_n_lps();

void follow_the_leader(simtime_t current_gvt)
{

	switch(current_phase) {
		case INIT:
			gvt_rounds = 0;
			free_rounds = 0;

            if(rid == gpu_rid) logger(LOG_INFO, "GPU has initialized its components");
			

			if(!__sync_add_and_fetch(&ftl_init_counter, -1)) {
				logger(LOG_INFO, "CPU has initialized its components");
				
                wall_clock_timer = gvt_timer = timer_new();
				gpu_gvt_timer = gvt_timer;
                if(ftl_transition(INIT, CHALLENGE))
                    logger(LOG_INFO, "Starting challenge %u", ++challenge_count);
                else
                    logger(LOG_FATAL, "Starting challenge %u FAILED", ++challenge_count);
				#if USE_DUMMY_CMP_SPEED	
						logger(LOG_WARN, "USING DUMMY CMP SPEED -- check if you really want this");
				#endif
            }
			
		
			while(current_phase == INIT); /// all threads should start a challenge by spinning, this should be fair
			return;

		case CHALLENGE:

			/// collect samples here
			if(WHO_AM_I == GPU_THREAD)
				register_gpu_data((double)(timer_new()-wall_clock_timer) / 1000000, current_gvt);
			if(WHO_AM_I == CPU_MAIN_THREAD)
				register_cpu_data((double)(timer_new()-wall_clock_timer) / 1000000, current_gvt);
			
			if((++gvt_rounds % FTL_PERIODS))
				return;

			/// the challenge is going to end
			gvt_rounds = 0;

			/// thread will spin waiting the result of the challenge
			/// this should reduce the impact on performance of the leaders w.r.t. going to sleep

            if(rid == gpu_rid) printf("\n");

			unsigned waiting = __sync_add_and_fetch(&ftl_curr_counter, -1);
            
			/// this is execute by the last cpu/gpu thread arriving here
			if(!waiting) {
				ftl_phase new_phase;
                printf("\n");
				
                logger(LOG_INFO, "RID-%u: the challenge is completed", rid);
				// printf("the barrier val should be always 0 : %u\n", ftl_curr_counter);

				
			#if USE_DUMMY_CMP_SPEED == 0
				if(is_cpu_faster()) { /// CPU WINS
			#else
				if(++dummyphase & 1){
			#endif       /// prepare next spin barrier after they wake up
                    reset_ftl_barrier(0,CPU_THREADS_CNT,0,1);
					new_phase = CPU;
				} else { /// GPU WINS
					new_phase = GPU;
				}

				logger(LOG_INFO, "RID-%u: the winner is %s\n",rid, new_phase == CPU ? "CPU" : "GPU");

				// Reset the time series, to avoid mixing data from different phases
				//reset_ftl_series();

				/// realing gvt timers for both cpu and gpu threads
				gvt_timer = timer_new();
				gpu_gvt_timer = gvt_timer;
                
                ftl_transition(CHALLENGE, new_phase);
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
                while(val > 1 && ftl_spin_barrier == 1)
					;
 
				if(val == 1) { /// i am the last CPU thread
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
				
				if(!val)
					logger(LOG_INFO, "HOST -> SIM: rootsim is aligned to %lf", actual_gvt);
					
				while(ftl_spin_barrier == 2);


				while(current_phase != CHALLENGE);

			} // all cpu threads go to sleep
            
            
            
			return;

		case GPU:
			register_gpu_data((double)(timer_new()-wall_clock_timer) / 1000000, current_gvt);
			break;
		case CPU:
			if(WHO_AM_I == CPU_MAIN_THREAD){
				register_cpu_data((double)(timer_new()-wall_clock_timer) / 1000000, current_gvt);
			}
			break;
		case END:
			break;
	}

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
                logger(LOG_INFO, "CPU phase endend\n");
				/// reinit spin barrier for CPU threads 				/// unlock all
                reset_ftl_barrier(0,CPU_THREADS_CNT,1,2);
			}

			align_device_to_host_parallel_states(rid, current_gvt);
            
			val = __sync_add_and_fetch(&ftl_curr_counter, -1);
			while(val && ftl_spin_barrier == 2); // all threads -1 will be stucked here


            if(!val) { /// i am the last one
                logger(LOG_INFO, "SIM -> HOST: all lps rollbacked to %lf", current_gvt);
                logger(LOG_INFO, "SIM -> HOST: transferred STATES (target lps %u)", get_n_lps());
				/// reinit spin barrier for CPU threads 				/// unlock all
                reset_ftl_barrier(0,CPU_THREADS_CNT,2,3);
			}

			align_device_to_host_parallel_events(rid, current_gvt);
            
            
			val = __sync_add_and_fetch(&ftl_curr_counter, -1);
			while(val && ftl_spin_barrier == 3); // all threads -1 will be stucked here

			
			if(val) {
				return;
			}

            logger(LOG_INFO, "SIM -> HOST: transferred EVENTS");
            logger(LOG_INFO, "SIM -> HOST: align completed");

			/// perform single threaded actions to alkign device to host
			align_device_to_host((int)current_gvt, n_blocks, threads_per_block);
			logger(LOG_INFO, "HOST -> DEVICE: gpu is aligned to %lf", current_gvt);
            
			__attribute__((fallthrough));

		case GPU:

			old_phase = current_phase;
			actual_gvt = current_gvt;
			
			if(current_phase == GPU){
				logger(LOG_INFO, "GPU phase endend\n");
			}

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
				logger(LOG_INFO, "DEVICE -> HOST: got all states and events");

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
			__sync_lock_test_and_set(&current_phase, CHALLENGE); /// unlock any thread spinning here
			logger(LOG_INFO, "Starting challenge %u %lf", ++challenge_count, current_gvt);


			return;

		case INIT:
		case CHALLENGE:
		case END:
			abort();
	}
}
