#include <test.h>

#include <lp/lp.h>

#define N_LPS 90
#define N_THREADS 16

uint64_t n_lps = N_LPS;

static int proc_calls[N_LPS];
static int mmem_calls[N_LPS];
static int lib_calls[N_LPS];

void process_lp_init(void){ proc_calls[current_lp - lps]++;}
void process_lp_fini(void){ proc_calls[current_lp - lps]--;}

void model_memory_lp_init(void){ mmem_calls[current_lp - lps]++;}
void model_memory_lp_fini(void){ mmem_calls[current_lp - lps]--;}

void lib_lp_init(uint64_t llid){ lib_calls[llid]++;}
void lib_lp_fini(void){ lib_calls[current_lp - lps]--;}

static int lp_test_init(void)
{
	n_threads = N_THREADS;
	lp_global_init();
	return 0;
}

static int lp_test_fini(void)
{
	int ret = 0;
	for(unsigned i = 0; i < N_LPS; ++i){
		ret -= proc_calls[i] != 0;
		ret -= mmem_calls[i] != 0;
		ret -= lib_calls[i] != 0;
	}
	lp_global_fini();
	return ret;
}

static int lp_test(unsigned thread_id)
{
	int ret = 0;
	core_init();
	lp_init();
	if(test_thread_barrier()){
		for(unsigned i = 0; i < N_LPS; ++i){
			ret -= proc_calls[i] != 1;
			ret -= mmem_calls[i] != 1;
			ret -= lib_calls[i] != 1;
		}
	}
	test_thread_barrier();
	lp_fini();
	return ret;
}


const struct _test_config_t test_config = {
	.test_name = "lp",
	.threads_count = N_THREADS,
	.test_init_fnc = lp_test_init,
	.test_fini_fnc = lp_test_fini,
	.test_fnc = lp_test
};
