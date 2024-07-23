/**
 * @file test/tests/visibility/visibility_weak.c
 *
 * @brief Test: accessing a weak symbol that is not overridden
 *
 * SPDX-FileCopyrightText: 2008-2023 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <core/core.h>

#include <string.h>

int main(void)
{
	return strcmp(core_version, "3.0.0");
}
