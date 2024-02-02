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
#include "model.h"
#include "settings.h"

__device__ static Nodes nodes;

__device__ static uint	population;
__device__ static int	lookahead;
__device__ static int	mean;


curandState_t *simulation_snapshot;

char malloc_nodes(uint n_nodes) {
	cudaError_t err;

	Nodes h_nodes;
	simulation_snapshot = (curandState_t*) malloc(sizeof(curandState_t)*n_nodes);
	if(!simulation_snapshot) {printf("no memory for HOST side model state\n"); exit(1); }
	
	err = cudaMalloc(&(h_nodes.cr_state), sizeof(curandState_t) * n_nodes);
	if (err != cudaSuccess) { return 0; }
	cudaMemcpyToSymbol(nodes, &h_nodes, sizeof(Nodes));

	return 1;
}

void copy_nodes_from_host(uint n_nodes) {
	Nodes h_nodes;
	cudaMemcpyFromSymbol(&h_nodes, nodes, sizeof(Nodes));
	cudaMemcpy(h_nodes.cr_state, simulation_snapshot, sizeof(curandState_t) * n_nodes, cudaMemcpyHostToDevice);
}


void copy_nodes_to_host(uint n_nodes) {
	Nodes h_nodes;
	cudaMemcpyFromSymbol(&h_nodes, nodes, sizeof(Nodes));
	cudaMemcpy(simulation_snapshot, h_nodes.cr_state, sizeof(curandState_t) * n_nodes, cudaMemcpyDeviceToHost);
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
	new_event.receiver = random(cr_state, g_n_nodes);
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

extern "C" void align_device_to_host_parallel(unsigned rid){
	int start = -1;
	int i;

	for(i=0;i<global_config.lps;i++){
		if(lid_to_rid(i) != rid) continue;
		if(lid_to_rid(i) != rid && start != -1) break;
		if(start == -1) start = i;
		curandState_t *state = (curandState_t*) get_lp_state_base_pointer(i);
		simulation_snapshot[i] = *state;
	}
	//printf("copying data from SIM to HOST by %u from %u to %u\n", rid, start, i);
}


extern "C" void align_device_to_host(int gvt, unsigned n_blocks, unsigned threads_per_block){
	copy_nodes_from_host(global_config.lps);
	cudaDeviceSynchronize();
	printf("aligned memory from HOST to DEVICE\n");
	
	kernel_init_queues<<<n_blocks, threads_per_block>>>();
	cudaDeviceSynchronize();
	printf("re init queues \n");
	
	kernel_reinit_nodes<<<n_blocks, threads_per_block>>>(gvt);
	cudaDeviceSynchronize();
	printf("re init nodes \n");
	
	kernel_sort_event_queues<<<n_blocks, threads_per_block>>>();
	cudaDeviceSynchronize();
	printf("sort queues \n");

}


extern "C" void align_host_to_device(int gvt){
	copy_nodes_to_host(global_config.lps);  
	cudaDeviceSynchronize();
}



extern "C" void align_host_to_device_parallel(int gvt){
	int start = -1;
	int i;
	for(i=0;i<global_config.lps;i++){
		if(lid_to_rid(i) != rid && start == -1) continue;
		if(lid_to_rid(i) != rid && start != -1) break;
		if(start == -1) start = i;
		curandState_t *state = (curandState_t*) get_lp_state_base_pointer(i);
		*state = simulation_snapshot[i];
		//process_device_align_msg(i, gvt);
	}

	//printf("copying data from HOST to SIM by %u from %u to %u\n", rid, start, i);
}

