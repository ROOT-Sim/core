/**
* @file test/core/visibility_override.c
*
* @brief Test: accessing a weak symbol that is not overridden
*
* SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
* SPDX-License-Identifier: GPL-3.0-only
*/
#include <string.h>

char *core_version = "This is a new variable";

int main(void)
{
	return strcmp(core_version, ROOTSIM_VERSION) == 0;
}
