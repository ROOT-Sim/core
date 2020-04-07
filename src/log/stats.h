#pragma once



enum stats_time_t {
#ifndef NEUROME_SERIAL
	STATS_ROLLBACK,
	STATS_MSG_ROLLBACKED,
#endif
	STATS_MSG_PROCESSED,
	STATS_NUM
};

extern void stats_time_start(enum stats_time_t this_stat);
extern void stats_time_take(enum stats_time_t this_stat);
extern void stats_dump(void);
