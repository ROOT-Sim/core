#include <ROOT-Sim.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <stdio.h>

//~ #define SPIKE_MAX 100

typedef struct synapse_t{
	double weight;
} synapse_t;

typedef struct neuron_params_t{
	double inv_tau_m; // [1/ms]
	double inv_C_m; // [1/pF] 1e-12
	double reset_potential; // [mV]
	double threshold; // [mV]
	double refractory_period; // [ms]
	double inv_tau_syn; // [1/ms]
} neuron_params_t;

struct neuron_helper_t {
	double Iext;
	double A0;
	double A2;
	double Icond;
	double self_spike_time; // This is the time at which the neuron self spikes
};

typedef struct neuron_state_t{
	struct neuron_helper_t *helper;
	unsigned long int times_fired;
	
	double membrane_potential; // [mV]
	double I; 		// [pA]
	
	simtime_t last_fired; // For refractory period
	simtime_t last_updated;
} neuron_state_t;

/* In here, the topology is defined through a series of calls to the functions
 * - NewSynapse
 * - NewProbe
 * - NewSpikeTrain
 * */
void SNNInitTopology(unsigned long int neuron_count);

/* Initialize the neuron */
void* NeuronInit(unsigned long int me);

/* Create a new synapse by asking for allocated memory that will be managed as synapse */
extern void* NewSynapse(unsigned long int src_neuron, unsigned long int dest_neuron, size_t syn_state_size, bool is_static, simtime_t delay);

/* neuron receives spike */
void NeuronHandleSpike(unsigned long int me, simtime_t now, double value, void* neuron_state);
/* neuron wakes after spiking succesfully */
void NeuronWake(unsigned long int me, simtime_t now, void* state);

/* synapse handles the spike by updating its state and *returning* the spike intensity */
double SynapseHandleSpike(simtime_t now, unsigned long int src_neuron, unsigned long int dest_neuron, synapse_t* synapse_state);

/* is the neuron done? */
bool NeuronCanEnd(unsigned long int me, neuron_state_t* snapshot);

/* A probed neuron has done something */
void ProbeRead(simtime_t now, unsigned long int monitored_neuron, const neuron_state_t* neuron_state);

/* Called at the end of the run to gather statistics */
void GatherStatistics(simtime_t now, unsigned long int neuron_id, const neuron_state_t* neuron_state);

extern void SendSpike(unsigned long int sender, simtime_t delivery_time);

extern void MaybeSpike(unsigned long int sender, simtime_t* spiketimes, unsigned int spike_count);

extern void MaybeSpikeAndWake(unsigned long int sender, simtime_t spiketime);

extern void NewProbe(unsigned long int target_neuron);

extern void NewSpikeTrain(unsigned long int target_count, unsigned long int target_neurons[], unsigned long int spike_count, double intensities[], simtime_t timings[]);

extern void* st_malloc(size_t size);
extern void st_free(void* ptr);
