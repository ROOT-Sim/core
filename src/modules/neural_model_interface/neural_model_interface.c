#include <modules/neural_model_interface/neural_model_interface.h>
#include <lib/random/xoroshiro.h>
#include <math.h>

// This only works if retractability is enabled!

//~ #define M_DEBUG                // Undefine to decrease verbosity

#ifdef M_DEBUG

# define printdbg(...) \
    printf(__VA_ARGS__)
#else
# define printdbg(...)
#endif

//~ #undef M_DEBUG

#define setCurLP(lp_id) (current_lp = &lps[lp_id])

__thread bool will_spike = 0;

extern void SetState_pr(void* state);
void schedule_local_spike(lp_id_t receiver, simtime_t timestamp,
	unsigned event_type, const void *payload, unsigned payload_size);
struct lp_msg* ScheduleMessage(lp_id_t receiver, simtime_t timestamp,
	unsigned event_type, const void *payload, unsigned payload_size);

extern bool is_on_curr_node(lp_id_t ID);

inline __neuron_s* fetch_LP_state(lp_id_t ID);
extern __neuron_s* fetch_LP_state(lp_id_t ID);
/* Check if synapse going from src_n to dst_n already exists */
bool synapse_exists(__neuron_s* lp_state, neuron_id_t dst_n);
void prune_synapses(lp_id_t lp_id);
void prune_all_synapses();
unsigned int compact_synapses(lp_id_t lp_id);
void compact_all_synapses();
inline __neuron_s* fetch_current_state();

inline void ProcessEvent_pr(lp_id_t me, simtime_t now, unsigned int event, const void* evt_content, unsigned int size, void* lp_state){
	ProcessEvent(me, now, event, evt_content, size, lp_state);
}

void ProcessEvent(lp_id_t me, simtime_t now, unsigned int event, const void* evt_content, unsigned int size, void* lp_state) {
	(void)size;
	__neuron_s* state = (__neuron_s*) lp_state;
	const event_t* content = (const event_t*) evt_content;
	
	will_spike = 0;

	switch(event) {

		case LP_INIT:
		{
			if (me == 0) {
				printdbg("Running with %lu LPs and %lu neurons\n", n_lps, n_lps);
			}
			
			NeuronHandleSpike_pr(me, now, 0.0, fetch_current_state()->neuron_state);
			
			break;
		}
		
		
		case SPIKE:
		{
			printdbg("[F] Spike directed to N%lu with value: %lf\n", me, content->value);
			
			NeuronHandleSpike_pr(me, now, content->value, state->neuron_state);
			
			break;
		}
		
		case MAYBESPIKE:
		{
			// Framework-provided retractable messages -> valid
			printdbg("[F - MS N%lu] Spiking!\n", me);
			SendSpike(me, now);
			
			if(state->is_probed){
				printdbg("[F - MS N%lu] Neuron is probed\n", me);
				ProbeRead_pr(now, me, state->neuron_state);
			}
			
			break;
		}
		
		case MAYBESPIKEWAKE:
		{
			// With framework-provided retractable messages, if you get the MAYBESPIKEWAKE, it's valid!
			printdbg("[F - MSW N%lu] Spiking!\n", me);
			SendSpike(me, now);
			
			printdbg("[F - MSW N%lu] Waking neuron\n", me);
			NeuronWake_pr(me, now, state->neuron_state);
			
			if(state->is_probed){
				printdbg("[F - MSW N%lu] Neuron is probed\n", me);
				ProbeRead_pr(now, me, state->neuron_state);
			}
			
			printdbg("[F - MSW N%lu] Done\n", me);
			
			break;
		}
		
		case LP_FINI:
		{
			GatherStatistics_pr(now, me, state->neuron_state);
			break;
		}
		case MODEL_INIT:
		case MODEL_FINI:
		{
			return;
		}
	}
	
	if(!will_spike){ // If the neuron did not spike as a result of this event, Deschedule the currently scheduled retractable event
		DescheduleRetractableEvent();
	}
}

bool CanEnd(lp_id_t me, const void *snapshot) {
	if (unlikely(((__neuron_s*)snapshot)->is_probed)){
		return NeuronCanEnd_pr(me, ((__neuron_s*)snapshot)->neuron_state);
	}
	return true;
}

// FIXME
/* Get the state of the current LP */
inline __neuron_s* fetch_current_state(){
	return current_lp->lib_ctx_p->state_s;
}
extern __neuron_s* fetch_current_state();

// FIXME
/* Get the state of lp with id ID, if it belongs to the current node */
inline __neuron_s* fetch_LP_state(lp_id_t ID){
	// If LP is on node but managed by another thread... You need to check before
	if (!is_on_curr_node(ID)){
		printdbg("LP %lu is not on current node", ID);
		return NULL; // LP is not on the current node
	}
	
	return lps[ID].lib_ctx_p->state_s;
}


inline bool is_on_curr_node(lp_id_t ID){
#ifdef ROOTSIM_MPI
	return (lid_to_nid(ID) == nid);
#else
	(void) ID;
	return 1;
#endif
}


inline bool is_on_curr_thread(lp_id_t ID){
	return is_on_curr_node(ID) ? (lid_to_rid(ID) == rid) : 0;
}
extern bool is_on_curr_thread(lp_id_t ID);

/* When the neuron wants to send the spike, it calls this */
void SendSpike(neuron_id_t sender, simtime_t spiketime){
	printdbg("[LP%lu]SendSpike\n", sender);
	
	__neuron_s* state = fetch_current_state();
	double value;
	event_t new_event;
	neuron_id_t target_n;
	__syn_t *syn;
	simtime_t delivery_time;
	
	printdbg("[LP%lu]Neuron has %lu synapses\n", sender, array_count(state->synapses));
	for(unsigned long int i=0; i < array_count(state->synapses); i++){
		
		syn = array_get_at(state->synapses, i);
		
		printdbg("[LP%lu] got synapse %lu. Target: N%lu\n", sender, i, syn->target);
		
		target_n = syn->target;
		
		delivery_time = spiketime + syn->delay;

		value = SynapseHandleSpike_pr(delivery_time, sender, target_n, syn->data);
		new_event.value = value;
		
		ScheduleNewEvent_pr(target_n, delivery_time, SPIKE, &new_event, sizeof(new_event));
	}
}

/* Neuron calls this when he wants to spike at some time t in the future,
 * but only if it does not receive any spikes in the meantime */
void MaybeSpike(lp_id_t sender, simtime_t spiketime){
	
	(void) sender;
	
	will_spike = 1;
	ScheduleRetractableEvent(spiketime, MAYBESPIKE);
	
}

/* Neuron calls this when he wants to spike at some time t in the future and then get woken up,
 * but only if it does not receive any spikes in the meantime. */
void MaybeSpikeAndWake(lp_id_t sender, simtime_t spiketime){
	
	printdbg("[F] MSW Requested by N%lu for time %lf\n", sender, spiketime);
	
	(void) sender;
	
	will_spike = 1;
	ScheduleRetractableEvent(spiketime, MAYBESPIKEWAKE);
	
}

/* Create a new synapse from src to dest.
 * + If synapse is dynamic it mallocs memory of size syn_state_size in the memory of the right LP
 * + If synapse is static, non-rollbackable memory is allocated
 * Then returns the pointer.
 * The delay is used by the framework for spike scheduling. */
void* NewSynapse(neuron_id_t src_neuron, neuron_id_t dest_neuron, size_t syn_state_size, bool is_static, simtime_t delay){
	// Maybe check if we can do this, or throw an error
	//if(neuron_module_topology_init_already_done) *esplodi*;
	
	if(!is_on_curr_thread(src_neuron)){// Fall through
		return NULL;
	}
	
	//~ printdbg("Creating a new synapse from %lu to %lu\n", src_neuron, dest_neuron);
	
	__neuron_s* target_lp_state = fetch_LP_state(src_neuron);
	
	if (target_lp_state==NULL){// This should not happen
		return NULL;
	}
	
	// Set current_lp to the lp that will manage the synapse so the mm mallocs correctly
	setCurLP(src_neuron);
	
	__syn_t *synapse;

	if(is_static){
		synapse = mm_alloc(offsetof(__syn_t, data) + syn_state_size);
	} else {
		synapse = malloc_mt(offsetof(__syn_t, data) + syn_state_size);
	}
	
	synapse->target = dest_neuron;
	synapse->delay = delay;

	//~ printdbg("NewSyn: Got LP %lu's state\n", src_neuron);

	array_push(target_lp_state->synapses, synapse);

	// Reset current LP to the zeroth of the thread
	setCurLP(lid_thread_first);
	
	return synapse->data;
}

// FIXME: this will be invoked once per thread. Make sure no doubled spikes are scheduled globally. The if check should take care of this
/* Create a new input spike train with its targets */
void NewSpikeTrain(unsigned long int target_count, neuron_id_t target_neurons[], unsigned long int spike_count, double intensities[], simtime_t timings[]){
	event_t new_event;
	printdbg("[F] Spike train. Scheduling %lu spikes towards %lu neurons (%lu spikes total)\n", spike_count, target_count, spike_count*target_count);
	
	for (neuron_id_t i=0; i<target_count; i++){
		if (is_on_curr_thread(target_neurons[i])){ // Only schedule events directed to this thread.
			for (unsigned long int j=0; j<spike_count; j++){
				new_event.value = intensities[j];
				
				// ScheduleNewEvent assumes that the msg is sent by cur_lp (or current_lp). This does not care
				schedule_local_spike(target_neurons[i], timings[j], SPIKE, &new_event, sizeof(new_event));
			}
		}
	}
}

/* Set a neuron to be probed */
void NewProbe(neuron_id_t target_neuron){
	
	if(!is_on_curr_thread(target_neuron)){// Fall through
		return;
	}
	
	__neuron_s* target_lp_state = fetch_LP_state(target_neuron);
	if (target_lp_state==NULL) return;
	/* I cannot write the memory of an LP owned by a different node.
	 * Thus, if the fetch_LP_state returns NULL it means "The LP is on another node".
	 * This should not happen however. */
	
	target_lp_state->is_probed = 1;
	 
	return;
}

void snn_module_lp_init(){// Init the neuron memory here (memory manager is up and running) rather than in ProcessEvent of INIT

	printdbg("[SNN M T%u] Initializing topology...\n", rid);
	printdbg("[SNN M T%u] Initializing neurons...\n", rid);
	
	__neuron_s* state;
	
	// Iterate on LPs owned by this thread
	for(lp_id_t lp_id = lid_thread_first; lp_id < lid_thread_end; lp_id++){
		
		setCurLP(lp_id);
		
		state = mm_alloc(sizeof(__neuron_s));
		//~ printdbg("[SNN module] Mallocd neuron %lu block state. Setting state...\n", lp_g_id);
		
		SetState_pr(state);
		
		//~ printdbg("[SNN module] State set\n");
		
		//state->synapses = mm_alloc(sizeof(void*) * state->synapse_count);
		array_init(state->synapses);
		
		state->is_probed = 0;
				
		state->neuron_state = NeuronInit_pr(lp_id);
		
		printdbg("[SNN M T%u] Neuron %lu initialized\n", rid, lp_id);
		
	}
	
	//~ printdbg("[SNN module] lp init with %lu LPS\n", n_lps);
	
	// Reset current LP to the zeroth of the thread
	setCurLP(lid_thread_first);
	
	// Set the RNG state (of the first LP, which is the current) to a fixed one and save the previous
	// Save
	uint64_t prev_rng_s[4];
	double prev_unif;
	
	struct lib_ctx *ctx = current_lp->lib_ctx_p;
	
	memcpy(prev_rng_s, ctx->rng_s, sizeof(uint64_t)*4);
	prev_unif = ctx->unif;
	
	// Init RNG with a fixed seed
	//~ random_init(current_lp->lib_ctx_p->rng_s, 0, 42);
	random_init(ctx->rng_s, 0, 42);
	ctx->unif=NAN;
	
	SNNInitTopology_pr(n_lps);
	//set the flag? Smth like: neuron_topology_init_already_done = 1;
	
	// Restore the RNG state to the previous one
	memcpy(ctx->rng_s, prev_rng_s, sizeof(uint64_t)*4);
	ctx->unif = prev_unif;
	
	//~ // Prune doubled synapses
	//~ prune_all_synapses();
	
	//~ compact_all_synapses();
	
	printdbg("[SNN M T%u] Synapses pruned and compacted\n", rid);
	
	return;
}

//~ int cmpsyns(const void* a, const void* b){
	//~ __syn_t* sa = (__syn_t*) a;
	//~ __syn_t* sb = (__syn_t*) b;
	
	//~ return (sa->target - sb->target);
	
//~ }

//~ void prune_all_synapses(){
	//~ for(lp_id_t lp_id = lid_thread_first; lp_id < lid_thread_end; lp_id++){		
		//~ prune_synapses(lp_id);
	//~ }
//~ }

//~ void prune_synapses(lp_id_t lp_id){
	//~ // This invokes memory operations. We don't want them to be executed from a different thread than the owner.
	//~ if(!is_on_curr_thread(lp_id)){
		//~ printdbg("[prune_synapses] LP%lu is not managed by thread %d\n", lp_id, rid);
		//~ return;
	//~ }
	//~ __neuron_s* state = fetch_LP_state(lp_id);
	//~ // Sort synapses for faster removal
	//~ qsort(array_items(state->synapses), array_count(state->synapses), sizeof(__syn_t*), cmpsyns);
	
	//~ __syn_t* syn;
	//~ __syn_t* syn_prev;
	
	//~ for(unsigned long int i=1; i < array_count(state->synapses); i++){
		//~ // i-th and previous synapse
		//~ syn = array_get_at(state->synapses, i);
		//~ syn_prev = array_get_at(state->synapses, i-1);
		
		//~ if(syn->target == syn_prev->target){
			//~ // Physically remove synapse
			//~ array_remove_at(state->synapses, i);
			//~ i--; // The array has shrunk! We want the same i next iteration
		//~ }
	//~ }
//~ }

//~ void compact_all_synapses(){
	//~ unsigned int saved = 0;
	//~ for(lp_id_t lp_id = lid_thread_first; lp_id < lid_thread_end; lp_id++){
		//~ saved += compact_synapses(lp_id);
	//~ }
	
	//~ printf("Saved %u slots by compacting\n", saved);
//~ }

//~ unsigned int compact_synapses(lp_id_t lp_id){
	//~ // This invokes memory operations. We don't want them to be executed from a different thread than the owner.
	//~ if(!is_on_curr_thread(lp_id)){
		//~ printdbg("[compact_synapses] LP%lu is not managed by thread %d\n", lp_id, rid);
		//~ return 0;
	//~ }
	//~ __neuron_s* state = fetch_LP_state(lp_id);
	//~ unsigned int initial_size = array_capacity(state->synapses);
	//~ array_compact(state->synapses);
	//~ return initial_size - array_capacity(state->synapses);
//~ }


/* This is a ScheduleNewEvent for spikes that do not originate from an LP (e.g. spiketrains).
 * Scheduling a message for an LP that lies on a different node = bad bad. Be careful using this */
void schedule_local_spike(lp_id_t receiver, simtime_t timestamp,
	unsigned event_type, const void *payload, unsigned payload_size){
			
	struct lp_msg *msg = msg_allocator_pack(receiver, timestamp, event_type,
		payload, payload_size);
	
	// Is this needed? msg_allocator_pack already calls this
	atomic_store_explicit(&msg->flags, 0U, memory_order_relaxed);
	
	msg_queue_insert(msg);	
}
