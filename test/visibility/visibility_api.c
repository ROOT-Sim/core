/**
* @file test/core/visibility_override.c
*
* @brief Test: accessing a weak symbol that is not overridden
*
* SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
* SPDX-License-Identifier: GPL-3.0-only
*/
void (*ScheduleNewEvent)(void);

void bar(void);

int main(void)
{
	ScheduleNewEvent = bar;
	return 0;
}
