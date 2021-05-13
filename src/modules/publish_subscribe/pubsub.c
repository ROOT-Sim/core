#include <modules/publish_subscribe/pubsub.h>

#ifdef PUBSUB

#include <limits.h>		// for CHAR_BIT

// For when the message is being handled at node level
#define n_stripped_payload_size(msg)	((msg)->pl_size - size_of_pubsub_info)
#define n_offset_of_children_count(msg)	(n_stripped_payload_size((msg)))
#define n_offset_of_children_ptr(msg)	(n_offset_of_children_count((msg)) + sizeof(size_t))
#define n_children_count(msg)		(*((size_t*) ((msg)->pl + n_offset_of_children_count((msg)))))
#define n_children_ptr(msg)		(*((lp_msg***) ((msg)->pl + n_offset_of_children_ptr((msg)))))

// For when the message is being handled at thread level
#define size_of_thread_pubsub_info	(sizeof(lp_entry_arr*) + size_of_pubsub_info)
#define t_stripped_payload_size(msg)	((msg)->pl_size - size_of_thread_pubsub_info)
#define t_offset_of_lp_arr(msg)		(t_stripped_payload_size((msg)))
#define t_offset_of_children_count(msg)	(t_offset_of_lp_arr((msg)) + sizeof(lp_entry_arr*))
#define t_offset_of_children_ptr(msg)	(t_offset_of_children_count((msg)) + sizeof(size_t))
#define t_lp_arr(msg)			(*((lp_entry_arr**) ((msg)->pl + t_offset_of_lp_arr((msg)))))
#define t_children_count(msg)		(*((size_t*) ((msg)->pl + t_offset_of_children_count((msg)))))
#define t_children_ptr(msg)		(*((lp_msg***) ((msg)->pl + t_offset_of_children_ptr((msg)))))

// Valid for both Node-level and Thread-level
#define offset_of_children_ptr(msg)	((msg)->pl_size - sizeof(lp_msg**))
#define children_ptr(msg)		(*((lp_msg***) ((msg)->pl + offset_of_children_ptr((msg)))))
#define offset_of_children_count(msg)	(offset_of_children_ptr(msg) - sizeof(size_t))
#define children_count(msg)		(*((size_t*) ((msg)->pl + offset_of_children_count((msg)))))

// For locally (same-node) generated Node-level messages
#define offset_of_child_for_mpi(msg)	(offset_of_children_count(msg) - sizeof(lp_msg*))
#define child_for_mpi(msg)		(*((lp_msg**) ((msg)->pl + offset_of_child_for_mpi((msg)))))

#define LP_ID_MSB (((lp_id_t) 1) << (sizeof(lp_id_t)*CHAR_BIT - 1))

extern __thread bool silent_processing;



typedef struct table_lp_entry_t{
	lp_id_t lid;
	void* data;
} table_lp_entry_t;

typedef dyn_array(table_lp_entry_t) lp_entry_arr;

typedef struct table_thread_entry_t{
	rid_t tid;
	lp_entry_arr lp_arr;
} table_thread_entry_t;

typedef dyn_array(table_thread_entry_t) t_entry_arr;

t_entry_arr *subscribersTable;
spinlock_t *tableLocks;

inline void mpi_pubsub_remote_msg_send(lp_msg *msg, nid_t dest_nid);

inline void mpi_pubsub_remote_anti_msg_send(lp_msg *msg, nid_t dest_nid);

void node_actually_antimessage(lp_msg *msg);

// OK
void pubsub_lib_lp_init(){
	current_lp->subnodes = mm_alloc(bitmap_required_size(n_nodes));
	bitmap_initialize(current_lp->subnodes, n_nodes);
	return;
}

// OK
void pubsub_lib_global_init(){ // Init hashtable and locks
	// Right now, the hashtable is simply an array[n_lps]
	//~ subscribersTable = mm_alloc(sizeof(dyn_array(table_thread_entry_t))*n_lps);
	subscribersTable = mm_alloc(sizeof(t_entry_arr)*n_lps);
	tableLocks = mm_alloc(sizeof(spinlock_t)*n_lps);
	
	for(lp_id_t i=0; i<n_lps; i++){
		//~ subscribersTable[i].items = NULL;
		array_count(subscribersTable[i])=0; // Unorthodox. Works
		spin_init(&(tableLocks[i]));
	}
	
	return;
}

inline void send_pubsub_msg_to_remote_nid(lp_msg *msg, nid_t dest_nid){
	if (likely(dest_nid != nid)){// Send to remote nodes
		mpi_pubsub_remote_msg_send(msg, dest_nid);
	}
}

inline void send_to_sub_nodes(lp_msg *msg){
	// send to all subscribers
	block_bitmap* subs = current_lp->subnodes;
	
	// Set the msg ID!!
	msg->msg_id = msg_id_get(msg, gvt_phase_get());
	
	inline void send_m_foreach_set_bit(nid_t nid){
		send_pubsub_msg_to_remote_nid(msg, nid);
	}
	
	size_t bmp_sz = bitmap_required_size(n_nodes);
	// Send a copy to all remote nodes that manage a subbed LP
	bitmap_foreach_set(subs, bmp_sz, send_m_foreach_set_bit);
}

// This function handles a pubsub message for the node.
void node_handle_published_message(lp_msg* msg){
	
	/*
	 * The parent message keeps track of its children by keeping
	 * an array of pointers in the payload.
	 * Contents of msg->pl: [*(msg->pl) | size_t | lp_msg** ]
	 */
	
	//~ dyn_array(table_thread_entry_t) threads = subscribersTable[msg->dest];
	t_entry_arr threads = subscribersTable[msg->dest];
	
	if(!array_count(threads)){ // The entry of the publisher LP does not exist
		n_children_count(msg) = 0;
		n_children_ptr(msg) = NULL;
		// Is this needed? Just to be sure.
		atomic_fetch_add_explicit(&msg->flags, MSG_FLAG_PROCESSED, memory_order_relaxed);
		// Cannot just free because an antimsg might come
		msg_allocator_free_at_gvt(msg);
		return;
	}
	
	// Here create the payload for the messages threads will receive
	size_t child_pl_size = 	n_stripped_payload_size(msg) +
				size_of_thread_pubsub_info;
	char* child_payload = mm_alloc(child_pl_size);
	// Visualization of *child_payload once populated:
	// Byte offsets	:v-0       		v-og_pl_size	v-(pl_size+sizeof(void*))	
	// Contents	:[ 	*(msg->pl) 	| 	&lp_arr	|	childCount	| lp_msg**	]
	
	// Copy the msg payload
	// Here msg->pl contains [og_pl, pubsub_info], so the size to copy is n_stripped_payload_size
	memcpy(child_payload, msg->pl, n_stripped_payload_size(msg));
	(*((lp_msg***) (child_payload + offset_of_children_ptr(msg)))) = NULL;
	(*((size_t*) (child_payload + offset_of_children_count(msg)))) = 0;
	// *child_payload right now:
	// Byte offsets	:v-0       		v-og_pl_size	v-(pl_size+sizeof(void*))	
	// Contents	:[ 	*(msg->pl) 	| 	garbage	|	0	|	NULL		]

	// One message per thread
	int n_ch_count = array_count(threads);
	
#ifdef ROOTSIM_MPI
	bool from_local_node = (msg->dest == (lp_id_t) current_lid);
	if(from_local_node){
		// Plus one message as blueprint for MPI
		n_ch_count++;
	}
#endif

	n_children_count(msg) = n_ch_count;
	n_children_ptr(msg) = mm_alloc(sizeof(lp_msg*) * n_ch_count);

	int it = 0;
	
#ifdef ROOTSIM_MPI
	if(from_local_node){
		
		// Create a clone to be sent to other nodes
		lp_msg *clone_msg = msg_allocator_alloc(
					n_stripped_payload_size(msg));
		
		memcpy(clone_msg, msg, n_stripped_payload_size(msg) + offsetof(lp_msg, pl));
		children_ptr(msg)[0] = clone_msg;
		++it;
		msg_allocator_free_at_gvt(clone_msg);
		
		send_to_sub_nodes(clone_msg);
		
	}
#endif
	
	// For each subscribed Thread
	for(int c=0; it < n_ch_count; it++, c++){
		// Check that message has not been antimessaged
		if(msg->flags & MSG_FLAG_ANTI){
			// Stop creating children. Write correct count
			n_children_count(msg) = it;
			break;
		}
		
		// *child_payload right now:
		// Byte offsets	:v-0       		v-og_pl_size	v-(og_pl_size+sizeof(void*))	
		// Contents	:[ 	*(msg->pl) 	| 	garbage	|	0	|	NULL	]
		// The info for pubsub will be filled out by the thread handling the messages
		
		table_thread_entry_t *t_entry = &array_get_at(threads, c);
		
		// Create child message
		// Target holds the target thread's tid
		lp_msg *child_msg = msg_allocator_pack(
					t_entry->tid,
					msg->dest_t, 
					msg->m_type,
					child_payload,
					child_pl_size
				);
		
		// Put pointer to the correct lp_arr in payload
		t_lp_arr(child_msg) = &(t_entry->lp_arr);
		
		atomic_store_explicit(&child_msg->flags, MSG_FLAG_PUBSUB, memory_order_relaxed);
		
		// Keep track of the child message for rollbacks!
		n_children_ptr(msg)[it] = child_msg;
		
		// Push child message into target thread's incoming queue
		pubsub_msg_queue_insert(child_msg);
		
	}
	
	mm_free(child_payload);
	
	// Done processing
	int flags = atomic_fetch_add_explicit(&msg->flags, MSG_FLAG_PROCESSED, memory_order_relaxed);
	
	if (flags & MSG_FLAG_ANTI){
		// If the message has been antimessaged in the meantime
		// Take action
		node_actually_antimessage(msg);
		// Free the message
		pubsub_msg_free(msg);
		return;
	}
	
	/* This msg is only a blueprint: it will never be put into
	 * past_msgs and eventually garbage collected. Free it at gvt */
	msg_allocator_free_at_gvt(msg);
	
	return;
}


/* In ProcessPublishedEvent the user creates the message resulting from
 * the published one. The message is created using ScheduleNewEvent.
 * This will schedule the generated event and will push a pointer to it
 * in past_msgs.
 * We pop that pointer from past_msgs (it does not belong there) and add
 * it to the children of the msg we are unpacking here.
 * */
// This function is called when a thread extracts a pubsub message from its queue
void thread_handle_published_message(lp_msg* msg){
	
	// Check if antimsgd to be safe
	if(msg->flags & MSG_FLAG_ANTI){
		thread_handle_published_antimessage(msg);
		return;
	}
	
	/*
	 * The parent message keeps track of its children by keeping
	 * an array of pointers in the payload.
	 * Contents of msg->pl once populated:
	 * [ pl	| &lp_arr	| childCount	| lp_msg**	]
	 * 
	 * Contents right now:
	 * [ pl	| &lp_arr	| 	0	| NULL		]
	 */
	
	lp_entry_arr lp_arr = *t_lp_arr(msg);
	
	// One message per subscription
	t_children_count(msg) = array_count(lp_arr);
	
	if(!array_count(lp_arr)){
		//~ t_children_ptr(msg) = NULL;
		
		// FIXME Is setting PROCESSED needed?
		/* If we do not set it, the msg is not put back in Q in case of an antimsg.
		 * Since it has no children and is already enqueued to be freed, nothing needs to be done.
		 * Granted, the antimsging function wuold do nothing anyway. Maybe we can avoid wasting a call.
		 * */
		atomic_fetch_add_explicit(&msg->flags, MSG_FLAG_PROCESSED, memory_order_relaxed);
		// Fossil collect when it's time
		msg_allocator_free_at_gvt(msg);
		// Cannot just free because antimessaging could happen
		return;
	}
	
	children_ptr(msg) = mm_alloc(sizeof(lp_msg*) * array_count(lp_arr));
	// Contents msg->pl now:
	// Byte offsets	:v-0    v-og_pl_size	v-(pl_size+sizeof(void*))	
	// Contents	:[ pl	| &lp_arr	| childCount	| lp_msg**	]
	
	// Here create the payload for the messages LPs will receive
	size_t original_pl_size = t_stripped_payload_size(msg);
	
	lp_msg* child_msg;
	
	// For each subscribed LP
	for(array_count_t i=0; i < array_count(lp_arr); i++){
		// Check that message has not been antimessaged
		if(msg->flags & MSG_FLAG_ANTI){
			// Stop creating children. Write correct count
			t_children_count(msg) = i;
			break;
		}
		
		table_lp_entry_t c_lp_entry = array_get_at(lp_arr, i);
		
		lp_id_t target_lid = c_lp_entry.lid; // Still dirty
		bool justSub = target_lid & LP_ID_MSB;
		target_lid = target_lid & ~LP_ID_MSB; // Clear leftmost bit
		
		// Check: Subscribe or SubscribeAndHandle?
		if(justSub){ // Regular subscribe
			// Just enqueue a clone of the message!
			child_msg = msg_allocator_pack(
					target_lid,
					msg->dest_t,
					msg->m_type,
					msg->pl,
					original_pl_size
				);
			
			atomic_store_explicit(&child_msg->flags, 0U, memory_order_relaxed);
			/* FIXME: self scheduled. Just enQ, no locks needed */
			msg_queue_insert(msg);
			
		} else { // SubscribeAndHandle
			// Set the current LP to the target lp's id
			lp_struct* this_lp = &lps[target_lid];
			current_lp = this_lp;
			
			struct process_data *proc_p = &current_lp->p;
			
			// Get user-provided data from entry
			void *usr_data = c_lp_entry.data;
			
			// > Invoke the handler with correct data and lp_id
			ProcessPublishedEvent_pr(
				current_lid,
				msg->dest_t,
				msg->m_type,
				msg->pl,
				original_pl_size,
				usr_data
			);
			
			// Flags and all initialized in ScheduleNewEvent
			
			// Pop the created message from sent_msgs
			child_msg = array_pop(proc_p->sent_msgs);
			
		}
		
		// Keep track of the child message
		//~ msg->pl[ t_offset_of_children_ptr(msg) ][i] = child_msg;
		t_children_ptr(msg)[i] = child_msg;
		
	}
	// Done processing
	int flags = atomic_fetch_add_explicit(&msg->flags, MSG_FLAG_PROCESSED, memory_order_relaxed);
	
	if (flags & MSG_FLAG_ANTI){
		// If the message has been antimessaged in the meantime
		// Take action
		thread_handle_published_antimessage(msg);
		// Free the message
		pubsub_msg_free(msg);
		return;
	}
	
	msg_allocator_free_at_gvt(msg);
	
	return;
}

inline void PublishNewEvent_pr(simtime_t timestamp, unsigned event_type, const void *payload, unsigned payload_size){
	PublishNewEvent(timestamp, event_type, *payload, payload_size);
}

// Here the logic that takes care to send the message to every node is implemented.
void PublishNewEvent(simtime_t timestamp, unsigned event_type, const void *payload, unsigned payload_size){
	if(silent_processing)
		return;

	struct process_data *proc_p = &current_lp->p;
	
	// A node-level pubsub message is created.
	// Then the MPI send wrapper makes sure to only send user data
	lp_msg *msg = msg_allocator_alloc(payload_size + size_of_pubsub_info);
	msg->dest = current_lid;
	msg->dest_t = timestamp;
	msg->m_type = event_type;
	
	// In current usage, it is actually unlikely
	if(likely(payload_size)){
		memcpy(msg->pl, payload, payload_size);
	}
	
	atomic_store_explicit(&msg->flags, MSG_FLAG_PUBSUB, memory_order_relaxed);
	
	node_handle_published_message(msg);
	array_push(proc_p->sent_msgs, msg);
}

// Called from within remote_msg_map.c or when antimessaging sent msgs
void node_handle_published_antimessage(lp_msg* msg){
		
	int flags = atomic_fetch_add_explicit(&msg->flags,
			MSG_FLAG_ANTI, memory_order_relaxed);
		
	// Is the positive handler finished?
	if(flags & MSG_FLAG_PROCESSED){
		// Carry out the antimessaging
		node_actually_antimessage(msg);
		// Do not free the message, it will bee freed on gvt
	}
	
	/* (else) The positive handler still isn't done.
	 * It will carry out the actual antimessaging
	 * */
	
	return;
}

extern void send_pubsub_anti_to_remote_nid(lp_msg *msg, nid_t dest_nid);
inline void send_pubsub_anti_to_remote_nid(lp_msg *msg, nid_t dest_nid){
	if (likely(dest_nid != nid)){// Send to remote nodes
		mpi_pubsub_remote_anti_msg_send(msg, dest_nid);
	}
}

void node_actually_antimessage(lp_msg *msg){
	
	// Carry out the antimessaging
	size_t child_count = n_children_count(msg);
	
	// TODO: make sure that child_count is correctly set by the
	// positive handler in case the handling is interrupted halfway
	lp_msg** children = n_children_ptr(msg);
	lp_msg* cmsg;
	
#ifdef ROOTSIM_MPI
	// If the message is from local node, send antimessages via MPI!
	if( (msg->dest == (lp_id_t) current_lid) && children){
		
		// Get the child used as blueprint to send to other nodes
		cmsg = children[0];
		
		// The first child is not a thread-level in this case
		children++;
		child_count--;
		
		// Update the ID
		msg_id_anti_phase_set(cmsg->msg_id, gvt_phase_get());
		
		inline void send_anti_foreach_set_bit(nid_t nid){
			send_pubsub_anti_to_remote_nid(cmsg, nid);
		}
		
		block_bitmap* subs = current_lp->subnodes;
		size_t bmp_sz = bitmap_required_size(n_nodes);
		// Send a copy to all remote nodes that manage a subbed LP
		bitmap_foreach_set(subs, bmp_sz, send_anti_foreach_set_bit);
	}
#endif
	
	// The message generated no thread-level children
	if(!child_count) return;
	
	// Now antimessage every child
	for(long unsigned int i=0; i<child_count; i++){
		cmsg = children[i];
		
		// Set flag to anti
		int cflags = atomic_fetch_add_explicit(
			&cmsg->flags, MSG_FLAG_ANTI,
			memory_order_relaxed);

		// Requeue if already processed
		if (cflags & MSG_FLAG_PROCESSED) {
			// cmsg->dest contains the tid of target thread
			pubsub_msg_queue_insert(cmsg);
			
		}
	}
}

// This is called when a pubsub message with ANTI flag set is extracted
void thread_handle_published_antimessage(lp_msg *msg){
	
	// msg comes from incoming queue => no race conditions
	if(!(msg->flags & MSG_FLAG_PROCESSED)){
		// Hasn't been processed yet. Just destroy it
		msg_allocator_free(msg);
		return;
	}
	// else
	// Antimessage the children
	int child_count = t_children_count(msg);
	lp_msg** children = t_children_ptr(msg);
	lp_msg* cmsg;
	
	for(int i=0; i<child_count; i++){
		cmsg = children[i];
		
		// Set flag to anti
		int cflags = atomic_fetch_add_explicit(
			&cmsg->flags, MSG_FLAG_ANTI,
			memory_order_relaxed);

		// Requeue if already processed
		if (cflags & MSG_FLAG_PROCESSED) {
			// FIXME: local Q. No need for locking
			msg_queue_insert(cmsg);
		}
		
	}
	
	return;
}

table_lp_entry_t new_lp_entry(lp_id_t sub_id, void* data, bool justSubscribe){
	if(justSubscribe){
		sub_id = sub_id | LP_ID_MSB;
	}
	// Create a new entry for the table
	table_lp_entry_t lp_entry = {
		.lid = sub_id,
		.data = data
	};
	return lp_entry;
}

table_thread_entry_t new_thread_entry(){
	// Create a new array to keep LPs of curr thread
	//~ dyn_array(table_lp_entry_t) lparr;
	lp_entry_arr lparr;
	array_init(lparr);
	// Create thread subscription entry
	table_thread_entry_t entry = {
		.tid = rid,
		.lp_arr = lparr
	};
	return entry;
}

// return the index of the thread or that of its successor (i.e. where it should be)
int seekInSortedArray(t_entry_arr arr, rid_t tid){
	size_t size = array_count(arr);
	if(!size) return -1;
	//~ int min = array_get_at(arr, 0);
	//~ int max = array_get_at(arr, size-1);
	int min = 0;
	int max = size-1;
	
	int next;
	rid_t curr;
	do {
		next = (max+min)/2;
		curr = array_get_at(arr, next).tid;
		if (tid < curr){
			max = next-1;
		} else if (tid > curr) {
			min = next+1;
		} else { // Found it
			break;
		}
	} while( max >= min );
	if (max<min) return min; // Did not find it!
	return next;
}

extern void add_subbed_node(lp_id_t sub_id, lp_id_t pub_id);
// Crashes if LP pub_id is not local
inline void add_subbed_node(lp_id_t sub_id, lp_id_t pub_id){
	nid_t sub_node_id = lid_to_nid(sub_id);
	// Set sub_node_id-th bit in &lps[pub_id]->subnodes
	bitmap_set(lps[pub_id].subnodes, sub_node_id);
}

// Subscribe LP subscriber_id to LP publisher_id
void SubscribeAndHandle(lp_id_t subscriber_id, lp_id_t dirty_publisher_id, void* data){
//New: void SubscribeAndHandle(lp_id_t dirty_subscriber_id, lp_id_t publisher_id, void* data){//size_t data_size
	// TODO: use MSB of subscriber_id (after changing it in Subscribe)
	lp_id_t publisher_id = dirty_publisher_id & ~LP_ID_MSB; // Clear leftmost bit
	//New: lp_id_t subscriber_id = dirty_subsciber_id & ~LP_ID_MSB; // Clear leftmost bit
	
	bool sub_is_local = lid_to_nid(subscriber_id)==nid && lid_to_rid[subscriber_id]==rid;
	bool pub_is_local = lid_to_nid(publisher_id)==nid && lid_to_rid[publisher_id]==rid;
	
	if(pub_is_local){ // If the publisher is managed by local thread
		add_subbed_node(subscriber_id, publisher_id);
	}
	
	if (!sub_is_local){ // Nothing else left to do
		return;
	}
	
	// Acquire the lock
	spin_lock(&(tableLocks[publisher_id]));
	
	// Ptr to dyn_array holding entries of threads subbed to pub_LP
	t_entry_arr *subbedThreads_p = &(subscribersTable[publisher_id]);
	
	int size = array_count(*subbedThreads_p);	
	int pos = 0;
	
	if (!size){ // Uninitialized subscribersTable entry
		// Init the dyn_array
		array_init(*subbedThreads_p);
		
		// Push a new thread entry in the array in table[pub_id]
		array_push(*subbedThreads_p, new_thread_entry());
		
		// pos is 0. Correct.
		
	} else { // Array has at least one thread entry
		// Is there an entry for the current thread?
		pos = seekInSortedArray(*subbedThreads_p, rid);
		
		if (pos == size){ // No entry - add at end of array
			// Add a new thread entry at the end of table[pub_id]
			array_push(*subbedThreads_p, new_thread_entry());
			// pos is size. Correct
			
		} else if (array_get_at(*subbedThreads_p, pos).tid!=rid){ // No entry - add in middle of array
			// Add a new thread entry at index pos of table[pub_id]
			array_add_at(*subbedThreads_p, pos, new_thread_entry());
			// pos is the correct index
		}
		// pos is the found index. Correct.
	}
	
	// Now we know that the entry is there, at index pos
	
	bool justSub = dirty_publisher_id & LP_ID_MSB;
	//New: bool justSub = dirty_subscriber_id & LP_ID_MSB;
	
	// Insert the new lp_entry into the thread entry's array
	array_push(
		array_get_at(*subbedThreads_p, pos).lp_arr,
		new_lp_entry(subscriber_id, data, justSub)
	);
	// (*subbedThreads_p)[pos] is the thread_entry of the current thread
	
	// Release the lock
	spin_unlock(&(tableLocks[publisher_id]));
	
	return;
}

void Subscribe(lp_id_t subscriber_id, lp_id_t publisher_id){
	// TODO: use MSB of subscriber_id and not publisher
	// Set most significant bit to mean it is a normal Subscribe
	SubscribeAndHandle(
		subscriber_id,
		publisher_id | LP_ID_MSB,
		NULL);
	//~ New, using sub_id:
	//~ SubscribeAndHandle(
		//~ subscriber_id | LP_ID_MSB,
		//~ publisher_id,
		//~ NULL);
	return;
}

// Free a pubsub msg
void pubsub_msg_free(lp_msg* msg){
	
	// children_ptr(msg) works for both node and thread-level
	lp_msg **c_ptr = children_ptr(msg);
	
	if(c_ptr){
		// Free the array pointing to children
		// Children will be freed independently
		mm_free(c_ptr);
	}

	msg_allocator_free(msg);
	return;
}

// Insert a thread-level pubsub message in queue
void pubsub_msg_queue_insert(lp_msg* msg){
	
	// msg->dest contains the thread id 
	unsigned dest_rid = msg->dest;
	struct queue_t *this_q = &mqueue(rid, dest_rid);
	spin_lock(&this_q->lck);
	heap_insert(this_q->q, msg_is_before, msg);
	spin_unlock(&this_q->lck);
	
}

// Only use this after setting the msg ID
inline void mpi_pubsub_remote_msg_send(lp_msg *msg, nid_t dest_nid)
{
	gvt_on_remote_msg_send(dest_nid);

	mpi_lock();
	MPI_Request req;
	
	MPI_Isend(msg, msg_bare_size(msg), MPI_BYTE,
			dest_nid, 0, MPI_COMM_WORLD, &req);

	MPI_Request_free(&req);
	mpi_unlock();
}

// Only use this after correctly setting the ID
inline void mpi_pubsub_remote_anti_msg_send(lp_msg *msg, nid_t dest_nid)
{
	gvt_on_remote_msg_send(dest_nid);

	mpi_lock();
	MPI_Request req;
	MPI_Issend(&msg->msg_id, sizeof(msg->msg_id), MPI_BYTE,
				dest_nid, 0, MPI_COMM_WORLD, &req);
	MPI_Request_free(&req);
	mpi_unlock();
}


#undef LP_ID_MSB

#undef children_ptr
#undef offset_of_children_ptr
#undef children_count
#undef offset_of_children_count
#undef n_stripped_payload_size
#undef n_offset_of_children_ptr
#undef n_offset_of_children_count
#undef n_children_count
#undef n_children_ptr
#undef size_of_thread_pubsub_info
#undef t_stripped_payload_size
#undef t_offset_of_lp_arr
#undef t_offset_of_children_ptr
#undef t_offset_of_children_count
#undef t_lp_arr
#undef t_children_count
#undef t_children_ptr

#endif
