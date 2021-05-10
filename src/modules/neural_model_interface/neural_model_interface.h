#include <core/core.h>
#include <lib/lib_internal.h>
#include <datatypes/array.h>
//#include <errno.h>

#include <lp/lp.h>

// for ScheduleLocalSpike
#include <lp/msg.h>

#define SPIKE 1 // Event definition
#define MAYBESPIKE 2
#define MAYBESPIKEWAKE 3

/* <INTERFACES> */
typedef lp_id_t neuron_id_t;

/* neuron sends the spike */
void SendSpike(neuron_id_t sender, simtime_t spiketime);

/* Neuron calls this when he wants to spike at some time t in the future,
 * but only if it does not receive any spikes in the meantime */
void MaybeSpike(neuron_id_t sender, simtime_t spiketime);
/* Neuron calls this when he wants to spike at some time t in the future and then get woken up,
 * but only if it does not receive any spikes in the meantime. */
void MaybeSpikeAndWake(neuron_id_t sender, simtime_t spiketime);

/* Initialize the neuron */
extern void* NeuronInit(neuron_id_t me);
extern void* NeuronInit_pr(neuron_id_t me);

/* Invoked in NetworkInit by modeller to create a synapse. Can be static (non-rollbacked) and can have delay */
//~ void* NewSynapse(unsigned long int src_neuron, unsigned long int dest_neuron, size_t syn_state_size);
void* NewSynapse(neuron_id_t src_neuron, neuron_id_t dest_neuron, size_t syn_state_size, bool is_static, simtime_t delay);

/* Invoked in NetworkInit by modeller to create a new input spike train with its targets */
void NewSpikeTrain(unsigned long int target_count, neuron_id_t target_neurons[], unsigned long int spike_count, double intensities[], simtime_t timings[]);

//~ /* For the future. Invoked in NetworkInit by modeller to create a new input current source */
//~ void NewCurrentSource(params);

/* Invoked in NetworkInit by modeller to create a new output probe */
void NewProbe(neuron_id_t target_neuron);

/* The probe is told to wake up and look at the state of the neuron.
 * Called after monitored_neuron receives an input, and after it fires. */
// Think: should we make it possible for this to be issued periodically?
extern void ProbeRead(simtime_t now, neuron_id_t monitored_neuron, const void* neuron_state);
extern void ProbeRead_pr(simtime_t now, neuron_id_t monitored_neuron, const void* neuron_state);

/* neuron receives spike */
extern void NeuronHandleSpike(neuron_id_t neuron_id, simtime_t now, double value, void* neuron_state);
extern void NeuronHandleSpike_pr(neuron_id_t neuron_id, simtime_t now, double value, void* neuron_state);
/* Wake the neuron after spiking (or something like this) */
extern void NeuronWake(neuron_id_t me, simtime_t now, void* neuron_state);
extern void NeuronWake_pr(neuron_id_t me, simtime_t now, void* neuron_state);

/* synapse handles the spike by updating its state and *returning* the spike intensity */
extern double SynapseHandleSpike(simtime_t now, neuron_id_t src_neuron, neuron_id_t dest_neuron, void* synapse_state);
extern double SynapseHandleSpike_pr(simtime_t now, neuron_id_t src_neuron, neuron_id_t dest_neuron, void* synapse_state);

/* is the neuron done? */
extern bool NeuronCanEnd(neuron_id_t me, void* snapshot);
extern bool NeuronCanEnd_pr(neuron_id_t me, void* snapshot);

/* Gather statistics about the run */
extern void GatherStatistics(simtime_t now, neuron_id_t neuron_id, const void* neuron_state);
extern void GatherStatistics_pr(simtime_t now, neuron_id_t neuron_id, const void* neuron_state);

/* This is what the user calls */
extern void SNNInitTopology(neuron_id_t n_neurons);
extern void SNNInitTopology_pr(neuron_id_t n_neurons);

/* </INTERFACES> */

typedef struct __syn_t{
	lp_id_t target;
	simtime_t delay;
	unsigned char data[];
} __syn_t;

typedef struct mbspk_str {
	unsigned long int maybe_spike_id;
} mbspk_str;

typedef struct __neuron_s{
	dyn_array(__syn_t *) synapses;
	void* neuron_state;
	bool is_probed;
} __neuron_s;

typedef struct event_content_t {
	double value;
} event_t;

/* Tentative: This is the function called by the framework to initialize this module */
void snn_module_lp_init();
