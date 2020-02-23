#include <parallel/parallel.h>

#include <core/core.h>
#include <lib/lib.h>

#include <pthread.h>

void parallel_thread_run(void *unused)
{
	core_thread_id_assign();

}

void parallel_global_init(void)
{
	msg_queue_global_init();
	lib_global_init();


	// TODO
}

void parallel_global_fini(void)
{
	msg_queue_global_fini();
	lib_global_fini();
	// TODO
}

int main(int argc, char **argv)
{
	init_args_parse(argc, argv);

	parallel_global_init();

	arch_thread_init(global_config.threads_cnt - 1, parallel_thread_run, NULL);

	parallel_thread_run(NULL);
}

void ScheduleNewEvent(unsigned receiver, simtime_t timestamp,
	unsigned event_type, const void *payload, unsigned payload_size)
{
	lp_msg *msg = msg_alloc(payload_size + sizeof(unsigned));
	msg->destination = receiver;
	msg->destination_time = timestamp;
	*((unsigned *) msg->payload) = event_type;
	memcpy(&msg->payload[sizeof(unsigned)], payload, payload_size);

	msg_queue_insert(msg);
}
