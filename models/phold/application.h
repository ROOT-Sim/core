#include <ROOT-Sim.h>
#include <math.h>
#include <stdlib.h>

<<<<<<< HEAD
#define MAX_BUFFERS 1000
#define MAX_BUFFER_SIZE 1024
#define TAU 1.5
#define SEND_PROBABILITY 0.05
#define ALLOC_PROBABILITY 0.2
#define DEALLOC_PROBABILITY 0.2

// Event types
enum {
	LOOP = INIT + 1,
	RECEIVE
};
=======

// Distribution values
#define NO_DISTR	0  // Not actually used, but 0 might mean 'false'
#define UNIFORM		1
#define EXPO	 	2

// Default values
#define WRITE_DISTRIBUTION	0.1
#define READ_DISTRIBUTION	0.1
#define MAX_SIZE		1024
#define MIN_SIZE		32
#define OBJECT_TOTAL_SIZE	16000
#define NUM_BUFFERS		3
#define TAU		        10.0
#define COMPLETE_ALLOC		5000
// Event types
#define ALLOC		11
#define DEALLOC 	2
#define LOOP		3
>>>>>>> origin/asym

<<<<<<< HEAD
<<<<<<< HEAD
#define COMPLETE_EVENTS 2000	// for the LOOP traditional case
#define LOOP_COUNT	1000
//#define FANOUT 1
=======
#define COMPLETE_EVENTS 100000	// for the LOOP traditional case
#define LOOP_COUNT	500
=======
#define COMPLETE_EVENTS 10000	// for the LOOP traditional case
#define LOOP_COUNT	250000
>>>>>>> origin/power


// This is the events' payload which is exchanged across LPs
typedef struct _event_content_type {
	int size;
} event_content_type;

>>>>>>> origin/energy

// This structure defines buffers lists
typedef struct _buffer {
	unsigned count; // the number of elements of data
	struct _buffer *next;
	unsigned *data; //synthetic data array
} buffer;

// LP simulation state
typedef struct _lp_state_type {
	unsigned int events;
	unsigned buffer_count;
	unsigned total_checksum;
	buffer *head;
} lp_state_type;

buffer* get_buffer(buffer *head, unsigned i);
unsigned read_buffer(buffer *head, unsigned i); //reads synthetic data and performs stupid hash
buffer* allocate_buffer(buffer *head, unsigned *data, unsigned count); //allocates a buffer with @count of elements, initialized with content of @data.
buffer* deallocate_buffer(buffer *head, unsigned i); //deallocate buffer at pos i



