#include <ROOT-Sim.h>
#include <core/core.h>
#include <core/output.h>
#include <lp/msg.h>

extern __thread bool silent_processing;
extern __thread struct lp_msg *current_msg;

void ScheduleOutput(unsigned output_type, const void *output_content, unsigned output_size)
{
	if(unlikely(global_config.serial)) {
		// Chiama direttamente il dispatcher
		return;
	}

	if(unlikely(silent_processing))
		return;

	char *content = mm_alloc(output_size);

	if(__builtin_expect(output_size && !content, 0)) {
		logger(LOG_FATAL, "Out of memory!");
		abort(); // TODO: this can be criticized as xmalloc() in gcc. We shall dump partial stats before.
	}
	memcpy(content, output_content, output_size);

	struct output_data data = {.type = output_type, .content = content, .size = output_size};

	output_array_t *outputs = current_msg->outputs;
	if(!outputs) {
		outputs = mm_alloc(sizeof(output_array_t)); // Alloc array and then init
		if(__builtin_expect(!outputs, 0)) {
			logger(LOG_FATAL, "Out of memory!");
			abort(); // TODO: this can be criticized as xmalloc() in gcc. We shall dump partial stats
			         // before.
		}
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

	array_count(*outputs) = 0;
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
