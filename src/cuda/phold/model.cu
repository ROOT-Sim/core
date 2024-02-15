/*  This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>. */


#include <cuda/cuda_gpu.h>
#include <cuda/queues.h>
#include <cuda/kernels.h>

#include <cuda/random.h>
#include "model.h"
#include "settings.h"


__device__ static Nodes nodes;
__device__ static uint	population;
__device__ static int	lookahead;
__device__ static int	mean;


extern uint events_per_node;
extern __device__ EQs	eq;
extern "C" uint get_n_nodes();
extern "C" uint get_n_lps();
extern "C" uint get_n_nodes_per_lp();
extern "C" uint get_n_blocks();



curandState_t *simulation_snapshot;
uint *sim_bo;
uint *sim_so;
uint *sim_uo;
uint *sim_ql;
Event *sim_events;

char malloc_nodes(uint n_nodes) {
	cudaError_t err;

	Nodes h_nodes;
	simulation_snapshot = (curandState_t*) malloc(sizeof(curandState_t)*n_nodes);
	if(!sim_bo) sim_bo = (uint*)malloc(sizeof(uint) * n_nodes);
	if(!sim_so) sim_so = (uint*)malloc(sizeof(uint) * n_nodes);
	if(!sim_uo) sim_uo = (uint*)malloc(sizeof(uint) * n_nodes);
	if(!sim_ql) sim_ql = (uint*)malloc(sizeof(uint) * n_nodes);
	if(!sim_events) sim_events = (Event*)malloc(sizeof(Event) * n_nodes * events_per_node);

	if(!simulation_snapshot) {printf("no memory for HOST side model state\n"); exit(1); }

	err = cudaMalloc(&(h_nodes.cr_state), sizeof(curandState_t) * n_nodes);
	if (err != cudaSuccess) { return 0; }
	cudaMemcpyToSymbol(nodes, &h_nodes, sizeof(Nodes));

	return 1;
}

void free_nodes() {
	Nodes h_nodes;
	cudaMemcpyFromSymbol(&h_nodes, nodes, sizeof(Nodes));
	cudaFree(h_nodes.cr_state);
}

__device__
void set_model_params(int params[], uint n_params) {
	population = params[0];
	lookahead = params[1];
	mean = params[2];
}

__device__
int get_lookahead() {
	return lookahead;
}

__device__
void init_node(uint nid) {
	curand_init(nid, 0, 0, &(nodes.cr_state[nid]));

	uint n_events = population / g_n_nodes;
	if (nid < population % g_n_nodes) { n_events += 1; }

	for (uint i = 0; i < n_events; i++) {
		Event event;
		event.type = 1;
		event.sender = nid;
		event.receiver = nid;
		event.timestamp = i;

		append_event_to_queue(&event);
	}
}


__device__
void reinit_node(uint nid, int gvt) {

	uint n_events = population / g_n_nodes;
	curandState_t *cr_state = &(nodes.cr_state[nid]);

	for (uint i = 0; i < n_events; i++) {
		Event new_event;
		new_event.type = 1;
		new_event.sender = nid;
		new_event.receiver = random(cr_state, g_n_nodes);
		new_event.timestamp = gvt + lookahead + random_exp(cr_state, mean);

		char res = append_event_to_queue(&new_event);
	}
}

__device__
static int current_gpu_model_phase = 0;

__device__
static uint get_receiver(uint me, curandState_t *cr_state, int now)
{
	int hot = (now / PHASE_WINDOW_SIZE) % 2;
    if(me == 0 && hot == 0 && current_gpu_model_phase == 1) {
	    current_gpu_model_phase = 0;
	    printf("GPU: ENTER HOT PHASE at wall clock time %f\n", 0.);
    } else if(me == 0 && hot == 1 && current_gpu_model_phase == 0) {
	    current_gpu_model_phase = 1;
	    printf("GPU: ENTER HOT PHASE at wall clock time %f\n", 0.);
    }

    if(current_gpu_model_phase == 0)
	    return random(cr_state, HOT_FRACTION * g_n_nodes);
    return random(cr_state, g_n_nodes);
}

__device__ // private
char handle_event_type_1(Event *event) {
	uint nid = event->receiver;

#if OPTM_SYNC == 1
	uint lpid = nid / g_nodes_per_lp;

	if (state_queue_is_full(lpid)) { return 12; }
	if (antimsg_queue_is_full(lpid)) { return 13; }
#endif

	curandState_t *cr_state = &(nodes.cr_state[nid]);

	State old_state;
	old_state.cr_state = *cr_state;

	Event new_event;
	new_event.type = 1;
	new_event.sender = nid;
	new_event.receiver = get_receiver(nid, cr_state, event->timestamp);
	new_event.timestamp = event->timestamp + lookahead +
		random_exp(cr_state, mean);

	char res = append_event_to_queue(&new_event);

	if (res == 0) {
		nodes.cr_state[nid] = old_state.cr_state;
		return 11;
	}

#if OPTM_SYNC == 1
	append_state_to_queue(&old_state, lpid);
	append_antimsg_to_queue(&new_event);
#endif

	return 1;
}

#if OPTM_SYNC == 1
__device__ // private
void reverse_event_type_1(Event *event) {
	uint nid = event->receiver;
	uint lpid = nid / g_nodes_per_lp;

	State *old_state = delete_last_state(lpid);
	nodes.cr_state[nid] = old_state->cr_state;

	Event *antimsg = delete_last_antimsg(lpid);
	undo_event(antimsg);
}
#endif

__device__
char handle_event(Event *event) {
	uint type = event->type;

	if (type == 1) {
		return handle_event_type_1(event);
	} else {
		return 0;
	}
}

#if OPTM_SYNC == 1
__device__
void roll_back_event(Event *event) {
	uint type = event->type;

	if (type == 1) {
		reverse_event_type_1(event);
	}
}

__device__
uint get_number_states(Event *event) {
	return 1;
}

__device__
uint get_number_antimsgs(Event *event) {
	return 1;
}
#endif

__device__
void collect_statistics(uint nid) {
	return;
}

__device__
void print_statistics() {
	printf("STATISTICS NOT AVAILABLE\n");
}

extern "C" {
#include <core/core.h>
#include <lp/expose_lp_state.h>
extern void process_device_align_msg(unsigned lid, simtime_t time);

}



void copy_nodes_from_host(uint n_nodes) {
	Nodes h_nodes;
	cudaMemcpyFromSymbol(&h_nodes, nodes, sizeof(Nodes));
	cudaMemcpy(h_nodes.cr_state, simulation_snapshot, sizeof(curandState_t) * n_nodes, cudaMemcpyHostToDevice);

	EQs h_eq;
	cudaMemcpyFromSymbol(&h_eq, eq, sizeof(EQs));
	cudaMemcpy(h_eq.bo,sim_bo, sizeof(uint) * n_nodes, cudaMemcpyHostToDevice);
	cudaMemcpy(h_eq.so,sim_so, sizeof(uint) * n_nodes, cudaMemcpyHostToDevice);
	cudaMemcpy(h_eq.uo,sim_uo, sizeof(uint) * n_nodes, cudaMemcpyHostToDevice);
	cudaMemcpy(h_eq.ql,sim_ql, sizeof(uint) * n_nodes, cudaMemcpyHostToDevice);
	cudaMemcpy(h_eq.events,  sim_events, sizeof(Event) * n_nodes * events_per_node, cudaMemcpyHostToDevice);

}


void copy_nodes_to_host(uint n_nodes) {
	Nodes h_nodes;
	cudaMemcpyFromSymbol(&h_nodes, nodes, sizeof(Nodes));
	cudaMemcpy(simulation_snapshot, h_nodes.cr_state, sizeof(curandState_t) * n_nodes, cudaMemcpyDeviceToHost);

	EQs h_eq;
	cudaMemcpyFromSymbol(&h_eq, eq, sizeof(EQs));

	cudaMemcpy(sim_bo, h_eq.bo, sizeof(uint) * n_nodes, cudaMemcpyDeviceToHost);
	cudaMemcpy(sim_so, h_eq.so, sizeof(uint) * n_nodes, cudaMemcpyDeviceToHost);
	cudaMemcpy(sim_uo, h_eq.uo, sizeof(uint) * n_nodes, cudaMemcpyDeviceToHost);
	cudaMemcpy(sim_ql, h_eq.ql, sizeof(uint) * n_nodes, cudaMemcpyDeviceToHost);
	cudaMemcpy(sim_events, h_eq.events, sizeof(Event) * n_nodes * events_per_node, cudaMemcpyDeviceToHost);
	cudaDeviceSynchronize();
	printf("transferring mem frm DEV t HST\n");

}



extern "C" int pack_and_insert_gpu_event(unsigned des_node, unsigned sen_node, int ts, unsigned type){
	uint gpu_lp = des_node/get_n_nodes_per_lp();
	uint idx = __sync_fetch_and_add(sim_ql + gpu_lp, 1);

	if(idx >= get_n_nodes_per_lp()*events_per_node){
		printf("adding more events that queue capacity\n");
		exit(1);
	}

	uint base = get_n_nodes_per_lp()*events_per_node*gpu_lp;

	Event *tgt = sim_events+base+idx;
	tgt->receiver = des_node;
	tgt->sender   = sen_node;
	tgt->timestamp = ts;
	tgt->type = type;
	return 1;
}


extern "C" void align_device_to_host_parallel_states(unsigned rid, simtime_t gvt){
	unsigned start;
	unsigned i;

	start = global_config.lps+1;
	for(i=0;i<global_config.lps;i++){
		if(lid_to_rid(i) != rid && start == (global_config.lps+1)) continue;
		if(lid_to_rid(i) != rid && start != (global_config.lps+1)) break;
		if(start == (global_config.lps+1)) start = i;
		align_lp_state_to_gvt(gvt,i);
		curandState_t *state = (curandState_t*) get_lp_state_base_pointer(i);
		simulation_snapshot[i] = *state;
	}

	clean_per_thread_queue();

	if(!rid){
		bzero(sim_events, sizeof(Event) * get_n_nodes() * events_per_node);
		bzero(sim_bo, sizeof(uint) * get_n_nodes());
		bzero(sim_so, sizeof(uint) * get_n_nodes());
		bzero(sim_uo, sizeof(uint) * get_n_nodes());
		bzero(sim_ql, sizeof(uint) * get_n_nodes());
	}

//	printf("A - copying events from SIM to HOST by %u from %u to %u \n", rid, start, i-1);
}




extern "C" void align_device_to_host_parallel_events(unsigned rid, simtime_t gvt){
	unsigned cnt_a = 0;
	unsigned cnt_c = 0;
	unsigned start = (global_config.lps+1);
	unsigned i;

	start = global_config.lps+1;
	cnt_a = 0;
	cnt_c = 0;
	for(i=0;i<global_config.lps;i++){
		if(lid_to_rid(i) != rid && start == (global_config.lps+1)) continue;
		if(lid_to_rid(i) != rid && start != (global_config.lps+1)) break;
		if(start == (global_config.lps+1)) start = i;

		cnt_a += estimate_transfer_per_lp_events_without_filter(i);
		cnt_c += transfer_per_lp_events(i,gvt);
	}

//	printf("B - copying events from SIM to HOST by %u from %u to %u : #events %u(%u)overall capacity %u\n", rid, start, i-1, cnt_c, cnt_a, events_per_node*get_n_nodes());

	transfer_per_thread_events(gvt);

}


extern "C" void align_device_to_host(unsigned threads_per_block){

	copy_nodes_from_host(global_config.lps);

	cudaDeviceSynchronize();
	//printf("aligned memory from HOST to DEVICE\n");

	kernel_sort_event_queues<<<get_n_blocks(), threads_per_block>>>();
	cudaDeviceSynchronize();
	//printf("sort queues \n");

}


extern "C" void align_host_to_device(){
	copy_nodes_to_host(global_config.lps);
	cudaDeviceSynchronize();
}




extern "C" void align_host_to_device_parallel(simtime_t gvt){
	unsigned start = (global_config.lps+1);
	unsigned i;
	for(i=0;i<global_config.lps;i++){
		if(lid_to_rid(i) != rid && start == (global_config.lps+1)) continue;
		if(lid_to_rid(i) != rid && start != (global_config.lps+1)) break;
		if(start == (global_config.lps+1)) start = i;
		curandState_t *state = (curandState_t*) get_lp_state_base_pointer(i);
		*state = simulation_snapshot[i];
		process_device_align_msg(i, gvt);
	}
	//printf("copying states from HOST to SIM by %u from %u to %u\n", rid, start, i-1);

	uint pushed_events= 0;
	if(rid < get_n_lps()){
		for(i=0;i<get_n_lps()/global_config.n_threads;i++){
			uint lp = rid * (get_n_lps()/global_config.n_threads) + i;
			uint zero_idx  = lp*get_n_nodes_per_lp()*events_per_node;
			uint base_idx  = sim_bo[lp];
			uint start_idx = sim_so[lp];
			uint end_idx   = sim_uo[lp];
			//if(rid == 0) printf("base %u start %u end %u size %u\n", base_idx, start_idx, end_idx, get_n_nodes_per_lp()*events_per_node);
			while(start_idx != end_idx){
				uint effective = (base_idx+start_idx) % (get_n_nodes_per_lp()*events_per_node);
				Event *cur = sim_events+zero_idx+effective;
				//printf("A scheduling for %u a message from %u at %u\n", cur->receiver, cur->sender, cur->timestamp);
				custom_schedule_from_gpu(gvt, cur->sender, cur->receiver, (simtime_t) cur->timestamp, cur->type, NULL, 0);
				start_idx++;
				pushed_events++;
			}
		}
		//printf("copying events from HOST to SIM by %u from %u to %u GPU #LPS %u -- events pushed %u\n",
		//	rid, rid * (get_n_lps()/global_config.n_threads),rid * (get_n_lps()/global_config.n_threads)+get_n_lps()/global_config.n_threads-1, get_n_lps(), pushed_events);
	}

}
