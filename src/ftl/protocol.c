#include <arch/thread.h>
#include <core/core.h>
#include <arch/timer.h>
#include <pthread.h>
#include <ftl/ftl.h>


/// This is the id of the GPU thread
static uint gpu_rid = 0;

/// This is the spin barrier for init phase  
static volatile unsigned ftl_init_counter;       

/// This is the sleep barrier for challenges and used by CPU threads during GPU phases 
static pthread_barrier_t xpu_barrier;

/// This is the sleep barrier used by GPU thread during CPU phases 
static pthread_barrier_t gpu_barrier;

/// This is a spin barrier to be used for synchonized steps
static volatile unsigned ftl_curr_counter;
static volatile unsigned ftl_spin_barrier = 1;

/// This is the current phase and a spin barrier to be used for synchonized steps
volatile ftl_phase current_phase = INIT;

extern timer_uint gvt_timer;
extern timer_uint gpu_gvt_timer;
extern uint threads_per_block;
extern uint n_blocks;


unsigned dummyphase = 1;


__thread unsigned gvt_rounds  = 0;
__thread unsigned free_rounds = 0;


unsigned end_gpu = 0;
unsigned end_cpu = 0;
unsigned both_ended = 0;

void cpu_ended(){ end_cpu = 1;}
void gpu_ended(){ end_gpu = 1;}

unsigned sim_can_end() { return both_ended; }

#define FTL_PERIODS 20

void set_gpu_rid(unsigned rid){ 
	gpu_rid = rid; 
	ftl_curr_counter = rid+1; 
	ftl_init_counter = rid+1; 
	
	pthread_barrier_init(&xpu_barrier, NULL, gpu_rid+1);
	pthread_barrier_init(&gpu_barrier, NULL, 2);
	
}

void align_device_to_host(int gvt, unsigned n_blocks, unsigned threads_per_block);
void align_device_to_host_parallel(unsigned rid, unsigned n_blocks, unsigned threads_per_block);
void destroy_all_queues(void);

void follow_the_leader(simtime_t current_gvt){
	switch(current_phase){
		case INIT:
			gvt_rounds = 0;
			free_rounds = 0;
			
			while(!rid && ftl_init_counter > 1);
			if(!rid){
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
			
			if((++gvt_rounds % FTL_PERIODS)) return;
			
			/// the challenge is going to end
			gvt_rounds = 0;
			
			/// thread will spin waiting the result of the challenge 
			/// this should reduce the impact on performance of the leaders w.r.t. going to sleep
			
			unsigned waiting = __sync_add_and_fetch(&ftl_curr_counter,-1);  
			
			/// this is execute by the last cpu/gpu thread arriving here
			if(!waiting){
				ftl_phase new_phase;
				printf("\nthe challenge is completed RID %u\n", rid);
				//printf("the barrier val should be always 0 : %u\n", ftl_curr_counter); 


				/// TODO here the code to decide the winner

				/// CPU WINS
				if(dummyphase++ & 2){
					/// prepare next spin barrier after they wake up
					__sync_lock_test_and_set(&ftl_curr_counter, gpu_rid-1);
					__sync_lock_test_and_set(&ftl_spin_barrier, 1);
					new_phase = CPU;
				}
				/// GPU WINS
				else{
					new_phase = GPU;
				}
				
				printf("the winner is %s\n", new_phase == CPU ? "CPU": "GPU");
				
				/// realing gvt timers for both cpu and gpu threads
				gvt_timer = timer_new();
				gpu_gvt_timer = gvt_timer; 
				__sync_lock_test_and_set(&current_phase, new_phase);
			}
			
			/// wait here the result of the challenge
			while(current_phase == CHALLENGE); 
			
			
			/// CPU wins GPU loses
			if( current_phase == CPU && rid == gpu_rid) { 
				/// gpu thread go to sleep
				pthread_barrier_wait(&gpu_barrier); 
				/// spin wait the init of the next challenge
				while(current_phase != CHALLENGE);
			} 
			
			/// CPU wins GPU loses
			if( current_phase == GPU && rid != gpu_rid) { 
				pthread_barrier_wait(&xpu_barrier); 

				destroy_all_queues();
				unsigned val = __sync_add_and_fetch(&ftl_curr_counter,-1);
				printf("spinning before 1 %u %u\n", rid, val);
				while(ftl_spin_barrier == 1);
				printf("end spinning before 1 %u\n", rid);
				
				
				
				while(current_phase != CHALLENGE);
				
			} // all cpu threads go to sleep

			return;
	}

	/// TODO maybe collect samples also here ??
	if((++free_rounds % FTL_PERIODS)) return;
	free_rounds = 0;

	switch(current_phase){
		case CPU:
				/// all threads except the master one 
				if(rid != 0)  {
					unsigned val = __sync_add_and_fetch(&ftl_curr_counter,-1);
					while(ftl_spin_barrier == 1);
					align_device_to_host_parallel(rid,n_blocks,threads_per_block);
					val = __sync_add_and_fetch(&ftl_curr_counter,-1);
					printf("spinning before 2 %u %u\n", rid, val);
					while(ftl_spin_barrier == 2);
					printf("end spinning before 2 %u\n", rid);
					return;
				}
		case GPU:

				ftl_phase old_phase = current_phase;
				
				printf("\n%s phase ended\n", current_phase == CPU ? "CPU": "GPU");
				printf("\nstarting a new challenge by rid %u curr %u\n", rid, ftl_curr_counter);

				end_cpu = end_cpu || end_gpu;
				end_gpu = end_cpu || end_gpu;
				both_ended = end_cpu && end_gpu;

				if(old_phase == CPU && rid == 0) {
					
					while(ftl_curr_counter); // this matters only for rid 0 (CPU)                
					if(!__sync_bool_compare_and_swap(&ftl_curr_counter, 0,  gpu_rid-1)) printf("MY CAS FAILED AND THIS SHOULD NEVER HAPPEN\n");  

					align_device_to_host_parallel(rid,n_blocks,threads_per_block);
					__sync_lock_test_and_set(&ftl_spin_barrier, 2);

					while(ftl_curr_counter); // this matters only for rid 0 (CPU)                

					printf("copied memory from CPU SIM to HOST\n");

					align_device_to_host(current_gvt,n_blocks,threads_per_block);
					
					pthread_barrier_wait(&gpu_barrier);

					
				}
				
				if(old_phase == GPU && rid == gpu_rid){
					__sync_lock_test_and_set(&ftl_spin_barrier, 1);
					if(!__sync_bool_compare_and_swap(&ftl_curr_counter, 0,  gpu_rid)) printf("MY CAS FAILED AND THIS SHOULD NEVER HAPPEN\n");  

					pthread_barrier_wait(&xpu_barrier); 

					while(ftl_curr_counter); // this matters only for rid 0 (CPU)                
			   
				}
				
				__sync_lock_test_and_set(&ftl_spin_barrier, 0);
				if(!__sync_bool_compare_and_swap(&ftl_curr_counter, 0,  gpu_rid+1)) printf("MY CAS FAILED AND THIS SHOULD NEVER HAPPEN\n");  
				gvt_timer = timer_new();
				gpu_gvt_timer = gvt_timer;
				__sync_lock_test_and_set(&current_phase, CHALLENGE);
				
				printf("\nPrevious phase was %s at %lf ended (%u)\n", old_phase == CPU ? "CPU":"GPU", current_gvt, rid);
				
			return;
   }
}


