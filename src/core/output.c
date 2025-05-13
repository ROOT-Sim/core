#include <ROOT-Sim.h>
#include <core/core.h>
#include <core/output.h>
#include <lp/msg.h>
#include <lp/process.h>

extern __thread bool silent_processing;
extern __thread struct lp_msg *current_msg;

void ScheduleOutput(unsigned output_type, const void *output_content, unsigned output_size)
{
	if(unlikely(global_config.serial)) {
		// Call the dispatcher directly
		// TODO
		return;
	}

	if(unlikely(silent_processing))
		return;

	char *content = mm_alloc(output_size);
	memcpy(content, output_content, output_size);

	struct output_data data = {.type = output_type, .content = content, .size = output_size};

	output_array_t *outputs = current_msg->outputs;
	if(!outputs) {
		outputs = mm_alloc(sizeof(output_array_t));
		array_init(*outputs);
		current_msg->outputs = outputs;
	}

	array_push(*outputs, data);
}

void execute_outputs(struct lp_msg *msg)
{
	output_array_t *outputs = msg->outputs;
	if(!outputs)
		return;

	for(array_count_t i = 0; i < array_count(*outputs); ++i) {
		struct output_data data = array_get_at(*outputs, i);
		global_config.perform_output(msg->dest, data.type, data.content, data.size);
		mm_free(data.content);
	}

	array_count(*outputs) = 0; // Necessary to avoid double freeing the array elements.
}

void free_msg_outputs(output_array_t *output_array)
{
	if(!output_array)
		return;

	for(array_count_t i = 0; i < array_count(*output_array); ++i) {
		struct output_data data = array_get_at(*output_array, i);
		mm_free(data.content);
	}

	array_fini(*output_array);
	mm_free(output_array);
}

void committed_output_on_rollback(struct lp_msg *msg)
{
	output_array_t *outputs = msg->outputs;
	if(!outputs)
		return;

	for(array_count_t i = 0; i < array_count(*outputs); ++i) {
		struct output_data data = array_get_at(*outputs, i);
		mm_free(data.content);
	}

	array_count(*outputs) = 0; // Necessary to avoid double freeing the array elements.
}

void execute_outputs_batch(struct lp_msg **msg_array, array_count_t size)
{
	// Since dyn_arrays are unnamed structs, we work with the items element
	for(array_count_t i = 0; i < size; i++) {
		struct lp_msg *marked_msg = msg_array[i];
		struct lp_msg *msg = unmark_msg(marked_msg);
		if(msg->dest_t > global_config.termination_time) {
			return;
		}
		if(is_msg_past(marked_msg))
			execute_outputs(msg);
	}
}
