#include <test.h>

#include <lp/lp.h>

#define N_LPS 90
#define N_THREADS 16

uint64_t n_lps = N_LPS;

static int proc_calls[N_LPS];
static int mmem_calls[N_LPS];
static int lib_calls[N_LPS];
static int trm_calls[N_LPS];

void *__wrap_malloc(size_t siz)
{
	(void) siz;
	return NULL;
}

void fossil_lp_collect(void){ }
void sync_thread_barrier(void){ }

void termination_lp_init(void){ trm_calls[current_lp - lps]++;}

void process_lp_init(void){ proc_calls[current_lp - lps]++;}
void process_lp_deinit(void){ proc_calls[current_lp - lps] += 10;}
void process_lp_fini(void){ proc_calls[current_lp - lps]--;}

void model_memory_lp_init(void){ mmem_calls[current_lp - lps]++;}
void model_memory_lp_fini(void){ mmem_calls[current_lp - lps]--;}

void lib_lp_init(void){ lib_calls[current_lp - lps]++;}
void lib_lp_fini(void){ lib_calls[current_lp - lps]--;}

static int lp_test_init(void)
{
	lp_global_init();
	return 0;
}

static int lp_test_fini(void)
{
	int ret = 0;
	for(unsigned i = 0; i < N_LPS; ++i){
		ret -= proc_calls[i] != 10;
		ret -= mmem_calls[i] != 0;
		ret -= lib_calls[i] != 0;
		ret -= trm_calls[i] != 1;
	}
	lp_global_fini();
	return ret;
}

static int lp_test(void)
{
	int ret = 0;
	lp_init();
	if(test_thread_barrier()){
		for(unsigned i = 0; i < N_LPS; ++i){
			ret -= proc_calls[i] != 1;
			ret -= mmem_calls[i] != 1;
			ret -= lib_calls[i] != 1;
			ret -= trm_calls[i] != 1;
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
