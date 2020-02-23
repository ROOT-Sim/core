#include <core/core.h>

#include <core/intrinsics.h>

__thread unsigned tid;

void core_thread_id_assign(void)
{
	static unsigned tid_helper = 0;
	tid = FETCH_AND_ADD(&tid_helper, 1);
}
