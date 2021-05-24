/**
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

// Default configuration parameters
#define CHANNELS_PER_CELL     1000
#define TA_BASE               1.2
#define TA_DURATION           120
#define TA_CHANGE             200.0
#define COMPLETE_CALLS        5000

// Workload configuration parameters
#define WEEKEND_FACTOR        1.3
#define EARLY_MORNING_FACTOR  0.5
#define MORNING_FACTOR        1.0
#define LUNCH_FACTOR          1.6
#define AFTERNOON_FACTOR      1.0
#define EVENING_FACTOR        1.5
#define NIGHT_FACTOR          0.2

// LVT is in seconds, so we need to convert it
#define HOUR            3600
#define DAY             (24 * HOUR)
#define WEEK            (7 * DAY)

// Definition of the time phases within each day
#define EARLY_MORNING   (6.5 * HOUR)
#define MORNING         (13 * HOUR)
#define LUNCH           (15 * HOUR)
#define AFTERNOON       (19 * HOUR)
#define EVENING         (21 * HOUR)

// Events
#define START_CALL	20
#define END_CALL	21
#define HANDOFF_LEAVE	30
#define HANDOFF_RECV	31

// Macros to manage channel availability as a bitmap
#define MSK 1ULL
#define BITS (sizeof(unsigned long long) * CHAR_BIT)
#define CHECK_CHANNEL(P,I) ( (P)[(I) / BITS] & (MSK << ((I) % BITS)) )
#define SET_CHANNEL(P,I) ( (P)[(I) / BITS] |= (MSK << ((I) % BITS)) )
#define RESET_CHANNEL(P,I) ( (P)[(I) / BITS] &= ~(MSK << ((I) % BITS)) )


// Accurate simulation based on the model in:
// S. Kandukuri and S. Boyd,
// "Optimal power control in interference-limited fading wireless channels with outage-probability specifications,"
// in IEEE Transactions on Wireless Communications, vol. 1, no. 1, pp. 46-55, Jan. 2002, doi: 10.1109/7693.975444.
#define CROSS_PATH_GAIN		0.00000000000005
#define PATH_GAIN		0.0000000001
#define MIN_POWER		3
#define MAX_POWER		3000
#define SIR_AIM			10


enum reason { START, HANDOFF };

// Event Structure
struct event_content {
	unsigned cell;
	int channel;
	double end_time;
	enum reason r;
};

// Structs to define the simulation state
struct channel {
	int channel_id;
	double fading;
	double power;
	struct channel *next;
};

struct lp_state {
	double lvt;
	double ta;

	unsigned int channel_counter;
	unsigned int arriving_calls;
	unsigned int complete_calls;
	unsigned int blocked_on_setup;
	unsigned int blocked_on_handoff;
	unsigned int handoff_leaving;
	unsigned int handoff_entering;

	unsigned long long *channel_state;
	struct channel channels;
};
