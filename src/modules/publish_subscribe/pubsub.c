#include <modules/publish_subscribe/pubsub.h>

#ifdef PUBSUB

#include <limits.h> // for CHAR_BIT
#include <lp/process.h>
#include <datatypes/pubsub_map.h>

#define LP_ID_MSB (((lp_id_t)1) << (sizeof(lp_id_t) * CHAR_BIT - 1))

#define PUBSUB_PRINT_TOPOLOGY false

bool bitmap_check_bit(struct block_bitmap *bitmap, int bit)
{
	return bitmap_check(bitmap, bit);
}

// File for logging pubsub messages
static __thread FILE *pubsub_msgs_logfile;

#define unmark_msg(msg_p)                                                      \
	((struct lp_msg *)(((uintptr_t)(msg_p)) & (UINTPTR_MAX - 3)))

// FIXME: when we free node level pubsubs during cleanup we currently
//  free thread-level children that are also in msg_queue (processed + antimsg)

// Holds thread-level pubsub messages
__thread struct pubsub_map remote_pubsubs_map;
__thread dyn_array(struct lp_msg *) free_on_gvt_pubsubs;

t_entry_arr *subscribersTable;
spinlock_t *tableLocks;
static __thread struct lp_msg *delivered_pubsub = NULL;

static void thread_actually_antimessage(struct lp_msg *msg);
extern void pubsub_thread_msg_free(struct lp_msg *msg);

void pprint_subscribers_table();
void pprint_subscribers_adjacency_list();

// OK
void pubsub_module_lp_init()
{
	current_lp->subnodes = mm_alloc(bitmap_required_size(n_nodes));
	bitmap_initialize(current_lp->subnodes, n_nodes);
	current_lp->n_remote_sub_nodes = 0;
}

void pubsub_module_lp_fini()
{
	mm_free(current_lp->subnodes);
}

// OK
/// Initializes structures needed for pubsub module
void pubsub_module_global_init()
{
	// Right now, the hashtable is simply an array[n_lps]
	subscribersTable = mm_alloc(sizeof(t_entry_arr) * n_lps);
	tableLocks = mm_alloc(sizeof(spinlock_t) * n_lps);

	for(lp_id_t i = 0; i < n_lps; i++) {
		//~ subscribersTable[i].items = NULL;
		array_count(subscribersTable[i]) = 0; // Unorthodox. Works
		spin_init(&(tableLocks[i]));
	}
}

void print_pubsub_topology_to_file()
{
	if(!PUBSUB_PRINT_TOPOLOGY)
		return;
	// Print two json files:
	// One is the subscribers table
	// The other is an array that at index i contains the list of
	// subscribers of LP i. Each MPI node prints its file containing its
	// portion of the information
	FILE *f;
	char *fname = malloc(strlen("SubscribersTable_.json") + 100);
	sprintf(fname, "SubscribersTable_%d.json", nid);
	f = fopen(fname, "w");
	free(fname);
	pprint_subscribers_table(f);
	fclose(f);

	fname = malloc(strlen("SubscribersAdjacencyList_.json") + 100);
	sprintf(fname, "SubscribersAdjacencyList_%d.json", nid);
	f = fopen(fname, "w");
	free(fname);
	pprint_subscribers_adjacency_list(f);
	fclose(f);
}

/// Last call to the pubsub module
void pubsub_module_global_fini()
{
#if LOG_LEVEL <= LOG_DEBUG
	log_log(LOG_DEBUG, "Starting pubsub_module_global_fini\n");

	// Free subscribersTable, its locks and its contents
	mm_free(tableLocks);
	for(lp_id_t pub_id = 0; pub_id < n_lps; pub_id++) {
		t_entry_arr t_arr = subscribersTable[pub_id];
		if(!array_count(t_arr)) {
			continue;
		}
		// Free lp_arrs in t_arr.items
		for(array_count_t t_index = 0; t_index < array_count(t_arr);
		    ++t_index) {
			lp_entry_arr lp_arr =
			    array_get_at(t_arr, t_index).lp_arr;
			// free each allocated element of lp_arr.items
			for(array_count_t l_index = 0;
			    l_index < array_count(lp_arr); ++l_index) {
				table_lp_entry_t lp_entry =
				    array_get_at(lp_arr, l_index);

				if(lp_entry.data) {
					mm_free(lp_entry.data);
				}
			}
			array_fini(lp_arr);
		}
		array_fini(t_arr);
	}
	mm_free(subscribersTable);

	log_log(LOG_DEBUG, "Ending pubsub_module_global_fini\n");
#endif
}

void pubsub_module_init()
{
	pubsub_map_init(&remote_pubsubs_map);
	array_init(free_on_gvt_pubsubs);
	if(PUBSUB_DUMP_MSGS) {
		// Open logging file
		char *fname = malloc(strlen("pubsub_msgs_n_t.log") + 100);
		sprintf(fname, "pubsub_msgs_n%d_t%d.log", nid, rid);
		pubsub_msgs_logfile = fopen(fname, "w");
		setvbuf(pubsub_msgs_logfile, NULL, _IOFBF, 4096);
		fprintf(pubsub_msgs_logfile, "LP, send_time\n");
		free(fname);
	}
}

void log_pubsub_msgs_to_file(struct lp_msg **msg_array, array_count_t size)
{
	if(!PUBSUB_DUMP_MSGS)
		return;
	// Since dyn_arrays are unnamed structs, we work with the items element
	for(array_count_t i = 0; i < size; i++) {
		struct lp_msg *msg = unmark_msg(msg_array[i]);
		if(msg->dest_t > global_config.termination_time) {
			return;
		}

		// Only log pubsubs that have not been undone
		if(is_pubsub_msg(msg)) {
			fprintf(pubsub_msgs_logfile, "%lu, %.15lf\n", msg->send,
			    msg->dest_t);
		}
	}
}

void pubsub_module_fini()
{
	for(array_count_t i = 0; i < array_count(free_on_gvt_pubsubs); ++i) {
		pubsub_msg_free(array_get_at(free_on_gvt_pubsubs, i));
	}
	array_fini(free_on_gvt_pubsubs);

	// This implements freeing of messages too, as the map becomes the
	// owner of the messages we put in it
	pubsub_map_fini(&remote_pubsubs_map);

	if(PUBSUB_DUMP_MSGS)
		fclose(pubsub_msgs_logfile);
}

/// This is called when a local LP publishes a message.
/// Sends message via MPI, and unpacks local copies.
void pub_node_handle_published_message(struct lp_msg *msg)
{
	/*
	 * The parent message keeps track of its children by keeping
	 * an array of pointers in the payload.
	 * Contents of msg->pl: [*(msg->pl) | size_t | lp_msg** ]
	 */

	t_entry_arr threads = subscribersTable[msg->dest];

	// One message per thread
	int n_ch_count = array_count(threads);

#ifdef ROOTSIM_MPI
	n_ch_count += current_lp->n_remote_sub_nodes;
#endif

	struct t_pubsub_info *n_pi = &msg->pubsub_info;

	n_pi->m_cnt = n_ch_count;
	n_pi->m_arr = mm_alloc(sizeof(*n_pi->m_arr) * n_ch_count);
#if LOG_LEVEL <= LOG_DEBUG
	n_pi->lp_arr_p = (void *)(uintptr_t)0xDEADBEEF;
#endif

	// Index of next index to fill in n_children_ptr(msg)
	int it = 0;

#ifdef ROOTSIM_MPI
	// Create and send messages to other nodes
	int subbed_nodes = current_lp->n_remote_sub_nodes;
	if(subbed_nodes) {
		struct block_bitmap *subs = current_lp->subnodes;
		nid_t dest_nid = 0;
		do {
			// while (!bitmap_check(subs, dest_nid))
			while(!bitmap_check_bit(subs, dest_nid))
				++dest_nid;

			struct lp_msg *mpi_msg =
			    msg_allocator_alloc(msg->pl_size);
			memcpy(mpi_msg, msg, msg_bare_size(msg));
			n_pi->m_arr[it] = mpi_msg;

			mpi_remote_msg_send(mpi_msg, dest_nid);

			++it;
			++dest_nid;
		} while(it < subbed_nodes);
	}

#endif

	// Visualization of *child_payload once populated:
	// Offsets	:v-0       		v-original_pl_size
	// Contents	:[ 	*(msg->pl) 	| 	&lp_arr	|
	// childCount	| lp_msg**	]

	// Create and enqueue messages for each subscribed Thread
	for(int c = 0; it < n_ch_count; ++it, ++c) {
		table_thread_entry_t *t_entry = &array_get_at(threads, c);

		struct lp_msg *child_msg = msg_allocator_alloc(msg->pl_size);
		memcpy(child_msg, msg, msg_bare_size(msg));
		// Target is the target thread's tid
		child_msg->dest = t_entry->tid;

		struct t_pubsub_info *t_pi = &child_msg->pubsub_info;
		t_pi->lp_arr_p = &t_entry->lp_arr;
		t_pi->m_cnt = 0;
		t_pi->m_arr = NULL;

		// *child_payload right now:
		// Byte offsets	:	v-0       		v-user_pl_size
		// Contents	:		[ 	*(msg->pl) 	|
		// lp_arr*
		// |	0	|	NULL	] The info for pubsub will be
		// filled out by the thread handling the messages

		child_msg->raw_flags = MSG_FLAG_PUBSUB;

		// Keep track of the child message for rollbacks!
		n_pi->m_arr[it] = child_msg;
		// Push child message into target thread's incoming queue
		pubsub_msg_queue_insert(child_msg);
	}
}

// Should be ok?
/// Called when MPI extracts a pubsub message
void sub_node_handle_published_message(struct lp_msg *msg)
{
	// On receiver nodes, the original message is only used to create
	// thread-level ones

	t_entry_arr threads = subscribersTable[msg->dest];

	// One message per thread
	int n_ch_count = array_count(threads);

	// Visualization of child's payload once populated:
	// Offsets	:	v-0       		v-msg->pl_size
	// Contents	:	[ 	*(msg->pl) 	| 	lp_arr* |
	// childCount	| lp_msg**	]

	// Create and enqueue messages for each subscribed Thread
	for(int c = 0; c < n_ch_count; c++) {
		table_thread_entry_t *t_entry = &array_get_at(threads, c);

		// Create child message
		// Target holds the target thread's tid
		struct lp_msg *child_msg = msg_allocator_alloc(msg->pl_size);
		memcpy(child_msg, msg, msg_bare_size(msg));
		// Target is the target thread's tid
		child_msg->dest = t_entry->tid;

		struct t_pubsub_info *pi = &child_msg->pubsub_info;
		pi->lp_arr_p = &t_entry->lp_arr;
		pi->m_cnt = 0;
		pi->m_arr = NULL;

		// *child_payload right now:
		// Byte offsets	:	v-0       		v-user_pl_size
		// Contents	:		[ 	*(msg->pl) 	|
		// lp_arr*
		// |	0	|	NULL	] The info for pubsub will be
		// filled out by the thread handling the messages

		child_msg->raw_flags += MSG_FLAG_PUBSUB;

		// Push child message into target thread's incoming queue
		pubsub_msg_queue_insert(child_msg);
	}

	/* This msg is only a blueprint: Free it right now */
	msg_allocator_free(msg);
}

/* In ProcessPublishedEvent the user creates the message resulting from
 * the published one. The message is created using ScheduleNewEvent.
 * This will schedule the generated event and will push a pointer to it
 * in sent_msgs.
 * We pop that pointer from sent_msgs (it does not belong there) and add
 * it to the children of the msg we are unpacking here.
 * */
// This function is called when a thread extracts a pubsub message from its
// queue
void thread_handle_published_message(struct lp_msg *msg)
{
	if(msg->raw_flags >> MSG_FLAGS_BITS) {
		// Check early antimsgs
		if(unlikely(pubsub_map_add(&remote_pubsubs_map, msg) != NULL)) {
			msg_allocator_free(msg);
			return;
		}
	} else {
		uint32_t flags = atomic_fetch_add_explicit(&msg->flags,
		    MSG_FLAG_PROCESSED, memory_order_relaxed);
		if(unlikely(flags & MSG_FLAG_ANTI)) {
			// local thread-level pubsubs are now owned by their
			// parent
			return;
		}
	}

	stats_take(STATS_MSG_PUBSUB, 1);

	/*
	 * The parent message keeps track of its children by keeping
	 * an array of pointers in the payload.
	 * Contents of msg->pl once populated:
	 * [ pl	| &lp_arr	| childCount	| lp_msg**	]
	 *
	 * Contents right now:
	 * [ pl	| &lp_arr	| 	0	| NULL		]
	 */

	struct t_pubsub_info *pi = &msg->pubsub_info;
	lp_entry_arr lp_arr = *pi->lp_arr_p;

	// One message per subscription
	pi->m_cnt = array_count(lp_arr);
	pi->m_arr = mm_alloc(sizeof(*pi->m_arr) * array_count(lp_arr));
	// Contents msg->pl now:
	// Byte offsets	:v-0    v-og_pl_size	v-(pl_size+sizeof(void*))
	// Contents	:[ pl	| &lp_arr	| childCount	| lp_msg**
	// ]

	// For each subscribed LP
	for(array_count_t i = 0; i < array_count(lp_arr); i++) {
		struct lp_msg *child_msg;
		table_lp_entry_t c_lp_entry = array_get_at(lp_arr, i);

		lp_id_t target_lid = c_lp_entry.lid; // Still dirty
		bool justSub = target_lid & LP_ID_MSB;
		target_lid = target_lid & ~LP_ID_MSB; // Clear leftmost bit

		// Check: Subscribe or SubscribeAndHandle?
		if(justSub) { // Regular subscribe
			// Just enqueue a clone of the message!
			child_msg = msg_allocator_pack(target_lid, msg->dest_t,
			    msg->m_type, msg->pl, msg->pl_size);

			child_msg->send = msg->send;
#if LOG_LEVEL <= LOG_DEBUG
			child_msg->send_t = msg->send_t;
#endif

			atomic_store_explicit(&child_msg->flags, 0U,
			    memory_order_relaxed);

			process_msg(child_msg);

		} else { // SubscribeAndHandle
			// Set the current LP to the target lp's id
			struct lp_ctx *this_lp = &lps[target_lid];
			current_lp = this_lp;

			// Get user-provided data from entry
			void *usr_data = c_lp_entry.data;

			// > Invoke the handler with correct data and lp_id
			// ProcessPublishedEvent_pr(
			ProcessPublishedEvent(current_lp - lps, msg->dest_t,
			    msg->m_type, msg->pl, msg->pl_size, usr_data);

			// Flags initialized in PubsubDeliver
			child_msg = delivered_pubsub;
			delivered_pubsub = NULL;

			child_msg->send = msg->send;
#if LOG_LEVEL <= LOG_DEBUG
			child_msg->send_t = msg->send_t;
#endif

			// Child message is now processed here
			process_msg(child_msg);
		}

		// Keep track of the child message
		pi->m_arr[i] = child_msg;
	}
}

// ok
inline void PublishNewEvent_pr(simtime_t timestamp, unsigned event_type,
    const void *payload, unsigned payload_size)
{
	PublishNewEvent(timestamp, event_type, payload, payload_size);
}

// ok?
// Here the logic that takes care to send the message to every node is
// implemented.
void PublishNewEvent(simtime_t timestamp, unsigned event_type,
    const void *payload, unsigned payload_size)
{
	if(silent_processing)
		return;

	struct process_data *proc_p = &current_lp->p;

	// A node-level pubsub message is created.
	struct lp_msg *msg = msg_allocator_alloc(payload_size);
	msg->dest = current_lp - lps;
	msg->dest_t = timestamp;
	msg->m_type = event_type;
	msg->send = current_lp - lps;
#if LOG_LEVEL <= LOG_DEBUG
	msg->send_t = proc_p->last_t;
#endif

	if(payload_size) {
		memcpy(msg->pl, payload, payload_size);
	}

	msg->raw_flags = 0;

	pub_node_handle_published_message(msg);

	// Do this after making copies, or it will dirty MPI messages!
	msg->raw_flags = MSG_FLAG_PUBSUB;
	array_push(proc_p->p_msgs, mark_msg_remote(msg));
}

inline void PubsubDeliver_pr(simtime_t timestamp, unsigned event_type,
    const void *event_content, unsigned event_size)
{
	PubsubDeliver(timestamp, event_type, event_content, event_size);
}

void PubsubDeliver(simtime_t timestamp, unsigned event_type,
    const void *event_content, unsigned event_size)
{
	if(delivered_pubsub) {
		printf(
		    "A PubSub message was being delivered twice! Exiting...\n");
		abort();
	}
	delivered_pubsub = msg_allocator_pack(current_lp - lps, timestamp,
	    event_type, event_content, event_size);

	// Message is directed to self.
	delivered_pubsub->raw_flags = 0U;
	delivered_pubsub->m_seq = 0U;
}

// Ok?
/// This carries out the antimessaging when the node received the antimessage
/// via MPI
void sub_node_handle_published_antimessage(struct lp_msg *msg)
{
	// This flag is only set here to propagate it to children messages
	msg->raw_flags += MSG_FLAG_PUBSUB;

	// On sub nodes, just create thread-level copies
	t_entry_arr threads = subscribersTable[msg->dest];

	// One message per thread
	int n_ch_count = array_count(threads);

	// For each subscribed Thread
	for(int c = 0; c < n_ch_count; c++) {
		table_thread_entry_t *t_entry = &array_get_at(threads, c);

		// Child message is a clone of the message
		// Target holds the target thread's tid
		struct lp_msg *child_msg = msg_allocator_alloc(0);
		memcpy(child_msg, msg, msg_anti_size());
		child_msg->dest = t_entry->tid;
		// Flags are already set upon reception

		// Push child message into target thread's incoming queue
		pubsub_msg_queue_insert(child_msg);
	}

	/* This msg is only a blueprint: Free it right now */
	msg_allocator_free(msg);
}

// Ok?
/// This carries out the antimessaging when the node is the one responsible for
/// the publisher LP
void pub_node_handle_published_antimessage(struct lp_msg *msg)
{
	struct t_pubsub_info *n_pi = &msg->pubsub_info;

	// Carry out the antimessaging
	// the message originated from the local node. More precisely, it came
	// from current_LP as this can only be called when rolling back
	struct lp_msg **children = n_pi->m_arr;
	int child_count = n_pi->m_cnt;
	int it = 0;

#ifdef ROOTSIM_MPI
	// Antimessage via MPI
	int subbed_nodes = current_lp->n_remote_sub_nodes;
	if(subbed_nodes) {
		struct block_bitmap *subs = current_lp->subnodes;
		nid_t dest_nid = 0;
		do {
			while(!bitmap_check(subs, dest_nid))
				++dest_nid;

			mpi_remote_anti_msg_send(children[it], dest_nid);
			++it;
			++dest_nid;
		} while(it < subbed_nodes);
	}
#endif

	// Now antimessage every local child by setting flag + requeueing
	for(; it < child_count; ++it) {
		struct lp_msg *cmsg = children[it];

		// Set anti flag
		int cflags = atomic_fetch_add_explicit(&cmsg->flags,
		    MSG_FLAG_ANTI, memory_order_relaxed);

		// Requeue if already processed
		if(cflags & MSG_FLAG_PROCESSED) {
			// cmsg->dest contains the tid of target thread
			pubsub_msg_queue_insert(cmsg);
		}
	}

	array_push(free_on_gvt_pubsubs, msg);
}

// This is called when a pubsub message with ANTI flag set is extracted
void thread_handle_published_antimessage(struct lp_msg *anti_msg)
{
	// Is the publisher local to node
	if(!(anti_msg->raw_flags >> MSG_FLAGS_BITS)) {
		if(anti_msg->raw_flags & MSG_FLAG_PROCESSED) {
			thread_actually_antimessage(anti_msg);
		}
		return;
	}

	// Did we process the positive copy of this message already?
	struct lp_msg *msg = pubsub_map_add(&remote_pubsubs_map, anti_msg);
	if(unlikely(msg == NULL)) {
		return;
	}

	// We did
	msg_allocator_free(anti_msg);

	msg->raw_flags += MSG_FLAG_ANTI;
	thread_actually_antimessage(msg);
}

/// Carries out antimessaging of thread-level pubsub message
static void thread_actually_antimessage(struct lp_msg *msg)
{
	stats_take(STATS_MSG_PUBSUB_ANTI, 1);
	struct t_pubsub_info *pi = &msg->pubsub_info;

	for(size_t i = 0; i < pi->m_cnt; i++) {
		struct lp_msg *cmsg = pi->m_arr[i];

		uint32_t cflags = atomic_fetch_add_explicit(&cmsg->flags,
		    MSG_FLAG_ANTI, memory_order_relaxed);

		if(cflags & MSG_FLAG_PROCESSED)
			msg_queue_insert(cmsg);
	}
	// 	This was needed for cleanup, to distinguish thread-level pubsubs
	// that 	are still in the incoming queue, and those that are not.
	//	TODO: remove once we figure out how we want to handle this
	// problem 	msg->raw_flags -= MSG_FLAG_PROCESSED;
}

// OK
table_thread_entry_t new_thread_entry(void)
{
	// Create a new array to keep LPs of curr thread
	lp_entry_arr lparr;
	array_init(lparr);
	// Create thread subscription entry
	table_thread_entry_t entry = {.tid = rid, .lp_arr = lparr};
	return entry;
}

// OK
// return the index of the thread or that of its successor (i.e. where it should
// be)
int seekInSortedArray(t_entry_arr arr, rid_t tid)
{
	int size = array_count(arr);
	if(!size)
		return -1;
	//~ int min = array_get_at(arr, 0);
	//~ int max = array_get_at(arr, size-1);
	int min = 0;
	int max = size - 1;

	int next;
	rid_t curr;
	do {
		next = (max + min) / 2;
		curr = array_get_at(arr, next).tid;
		if(tid < curr) {
			max = next - 1;
		} else if(tid > curr) {
			min = next + 1;
		} else { // Found it
			break;
		}
	} while(max >= min);
	if(max < min)
		return min; // Did not find it!
	return next;
}

extern void add_subbed_node(lp_id_t sub_id, lp_id_t pub_id);
// OK
// Crashes if LP pub_id is not node-local. Data race if LP pub_id is not owned
// by local thread
inline void add_subbed_node(lp_id_t sub_id, lp_id_t pub_id)
{
	nid_t sub_node_id = lid_to_nid(sub_id);
	if(!bitmap_check(lps[pub_id].subnodes, sub_node_id)) {
		bitmap_set(lps[pub_id].subnodes, sub_node_id);
		lps[pub_id].n_remote_sub_nodes++;
		// Can also do this with bitmap_count_set after initialization
	}
}

// OK
// Subscribe LP subscriber_id to LP publisher_id
void SubscribeAndHandle(lp_id_t dirty_subscriber_id, lp_id_t publisher_id,
    void *data)
{
	lp_id_t subscriber_id =
	    dirty_subscriber_id & ~LP_ID_MSB; // Clear leftmost bit

	bool sub_is_node_local = lid_to_nid(subscriber_id) == nid;
	bool sub_is_thread_local =
	    sub_is_node_local && lid_to_rid(subscriber_id) == rid;
	bool pub_is_local =
	    lid_to_nid(publisher_id) == nid && lid_to_rid(publisher_id) == rid;


	if(pub_is_local && !sub_is_node_local) {
		// If the publisher is managed by local thread and sub is remote
		add_subbed_node(subscriber_id, publisher_id);
	}

	if(!sub_is_thread_local) {
		return;
	}

	// Acquire the lock
	spin_lock(&(tableLocks[publisher_id]));

	// dyn_array holding entries for threads subbed to pub_LP
	t_entry_arr *subbedThreads_p = &(subscribersTable[publisher_id]);

	int size = array_count(*subbedThreads_p);
	int pos = 0;

	if(!size) { // Uninitialized subscribersTable entry
		array_init(*subbedThreads_p);

		// Push a new thread entry in the array in table[pub_id]
		array_push(*subbedThreads_p, new_thread_entry());

		// pos is 0. Correct.

	} else { // Array has at least one thread entry
		// Is there an entry for the current thread?
		pos = seekInSortedArray(*subbedThreads_p, rid);

		if(pos == size) { // No entry - add at end of array
			// Add a new thread entry at the end of table[pub_id]
			array_push(*subbedThreads_p, new_thread_entry());
			// pos is size. Correct

		} else if(array_get_at(*subbedThreads_p, pos).tid !=
			  rid) { // No entry - add in middle of array
			// Add a new thread entry at index pos of table[pub_id]
			array_add_at(*subbedThreads_p, pos, new_thread_entry());
			// pos is the correct index
		}
	}

	// Thread's entry is at index pos

	// Insert lp_entry in the thread's array
	table_lp_entry_t lp_entry = {.lid = dirty_subscriber_id, .data = data};

	array_push(array_get_at(*subbedThreads_p, pos).lp_arr, lp_entry);

	// Release the lock
	spin_unlock(&(tableLocks[publisher_id]));
}

// OK
void Subscribe(lp_id_t subscriber_id, lp_id_t publisher_id)
{
	// Set most significant bit of subscriber_id to indicate it is a normal
	// Subscribe
	SubscribeAndHandle(subscriber_id | LP_ID_MSB, publisher_id, NULL);
}

// Free a (node level) pubsub msg
void pubsub_msg_free(struct lp_msg *msg)
{
	struct t_pubsub_info *pi = &msg->pubsub_info;

	// If it has children
	size_t children_ct = pi->m_cnt;
	// We can use count of local subscribers to know
	// how many children were destined to MPI
	if(likely(children_ct)) {
		array_count_t local_children =
		    array_count(subscribersTable[msg->dest]);
		array_count_t remote_children = children_ct - local_children;
		// Free children
		for(size_t i = 0; i < children_ct; i++) {
			if(i < remote_children) {
				msg_allocator_free(pi->m_arr[i]);
			} else {
				pubsub_thread_msg_free(pi->m_arr[i]);
			}
		}
	}
	// Free the array pointing to children
	mm_free(pi->m_arr);
	msg_allocator_free(msg);
}

void pubsub_thread_msg_free(struct lp_msg *msg)
{
	mm_free(msg->pubsub_info.m_arr);
	msg_allocator_free(msg);
}

// Pubsub free to be used by msg_queue_fini.
void pubsub_thread_msg_free_in_fini(struct lp_msg *msg)
{
	// msg is local
	if(!(msg->raw_flags >> MSG_FLAGS_BITS)) {
		return;
	}
	// Only free remote  messages (=>unprocessed)
	msg_allocator_free(msg);
}

// OK
// Insert a thread-level pubsub message in queue
void pubsub_msg_queue_insert(struct lp_msg *msg)
{
	// msg->dest contains the thread id
	unsigned dest_rid = msg->dest;
	struct msg_queue *mq = &queues[dest_rid];
	struct q_elem qe = {.t = msg->dest_t, .m = msg};

	spin_lock(&mq->q_lock);
	array_push(mq->b, qe);
	spin_unlock(&mq->q_lock);
}

void pubsub_on_gvt(simtime_t current_gvt)
{
	// Fossil collect free_on_gvt_pubsubs
	for(array_count_t i = array_count(free_on_gvt_pubsubs); i--;) {
		struct lp_msg *msg = array_get_at(free_on_gvt_pubsubs, i);
		if(msg->dest_t >= current_gvt)
			continue;
		pubsub_msg_free(msg);
		array_get_at(free_on_gvt_pubsubs, i) =
		    array_pop(free_on_gvt_pubsubs);
	}

	pubsub_map_fossil_collect(&remote_pubsubs_map, current_gvt);
}

#if LOG_LEVEL <= LOG_DEBUG
void pprint_table_lp_entry_t(table_lp_entry_t e, FILE *f)
{
	fprintf(f, "%lu", e.lid);
}

// typedef dyn_array(table_lp_entry_t) lp_entry_arr;
void pprint_lp_entry_arr(lp_entry_arr a, FILE *f)
{
	// è un dyn_array di table_lp_entry_t
	size_t ct = array_count(a);
	fprintf(f, "[");
	for(size_t i = 0; i < ct; i++) {
		pprint_table_lp_entry_t(array_get_at(a, i), f);
		if(i + 1 < ct)
			fprintf(f, ", ");
	}
	fprintf(f, "]\n");
}

void pprint_table_thread_entry_t(table_thread_entry_t e, FILE *f)
{
	fprintf(f, "\t\t");
	fprintf(f, "{\n");
	fprintf(f, "\t\t\t\"Tid\": %u,\n\t\t\t\"LP_arr\": ", e.tid);
	pprint_lp_entry_arr(e.lp_arr, f);
	fprintf(f, "\t\t");
	fprintf(f, "}");
}

void pprint_t_entry_arr(t_entry_arr a, FILE *f)
{
	fprintf(f, "\t");
	fprintf(f, "[\n");
	// è un dyn_array di table_thread_entry_t
	size_t ct = array_count(a);
	for(size_t i = 0; i < ct; i++) {
		pprint_table_thread_entry_t(array_get_at(a, i), f);
		if(i + 1 < ct)
			fprintf(f, ",");
		fprintf(f, "\n");
	}
	fprintf(f, "\t");
	fprintf(f, "]");
}

// Print the table to a file
void pprint_subscribers_table(FILE *f)
{
	t_entry_arr *t = subscribersTable;
	fprintf(f, "[\n");
	// Size of t: n_lps;
	for(lp_id_t i = 0; i < n_lps; i++) {
		// Entry dei sub dell'LP con id "i": t[i];
		pprint_t_entry_arr(t[i], f);
		if(i + 1 < n_lps)
			fprintf(f, ",");
		fprintf(f, "\n");
	}
	fprintf(f, "]\n");
}

void pprint_subscribers_adjacency_list(FILE *f)
{
	// Subscribers table
	t_entry_arr *t = subscribersTable;
	fprintf(f, "[\n");
	// Size of t: n_lps;
	for(lp_id_t pub_id = 0; pub_id < n_lps; pub_id++) {
		// Working with subscribers table entries, which are dyn_arrays
		// of t_entry
		t_entry_arr t_entry_arr = t[pub_id];

		fprintf(f, "\t[");

		size_t t_ct = array_count(t_entry_arr);
		for(size_t t_i = 0; t_i < t_ct; t_i++) {
			// Now we get the lp_arr in t_entry, which are
			// dyn_arrays of lp_entry
			lp_entry_arr lp_arr =
			    array_get_at(t_entry_arr, t_i).lp_arr;

			size_t lp_ct = array_count(lp_arr);
			for(size_t l_i = 0; l_i < lp_ct; l_i++) {
				fprintf(f, "%lu",
				    array_get_at(lp_arr, l_i).lid);
				if(t_i + 1 < t_ct || l_i + 1 < lp_ct)
					fprintf(f, ", ");
			}
		}

		fprintf(f, "]");
		if(pub_id + 1 < n_lps)
			fprintf(f, ",");
		fprintf(f, "\n");
	}
	fprintf(f, "]\n");
}
#endif

#endif
