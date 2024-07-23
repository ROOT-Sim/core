/**
 * @file test/tests/visibility/visibility_override.c
 *
 * @brief Test: accessing a weak symbol that is overridden
 *
 * SPDX-FileCopyrightText: 2008-2023 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <string.h>

#define VERSION_OVERRIDE_STR "This is a new variable"

char *core_version = VERSION_OVERRIDE_STR;

int main(void)
{
	return strcmp(core_version, VERSION_OVERRIDE_STR);
}
