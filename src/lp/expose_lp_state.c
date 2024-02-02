#include <lp/lp.h>

void* get_lp_state_base_pointer(unsigned int i){
    return lps[i].state_pointer;
} 
