/**
* @file test/mock.c
*
* @brief Mocking module
*
* This module allows to mock various parts of the core for testing purposes
*
* SPDX-FileCopyrightText: 2008-2025 HPDCS Group <rootsim@googlegroups.com>
* SPDX-License-Identifier: GPL-3.0-only
*/
#include <mock.h>

/**
 * @brief Mock structure for representing a logical process (LP) context.
 *
 * This structure is used to mock the logical process context for testing purposes.
 */
struct lp_mock {
	struct lp_ctx lp; /**< The logical process context. */
};

/**
 * @brief Thread-local instance of the mocked logical process context.
 *
 * This thread-local variable provides a unique instance of the mocked LP context
 * for each thread.
 */
static _Thread_local struct lp_mock lp_mock;

/**
 * @brief Retrieves the mocked logical process context.
 *
 * This function returns a pointer to the thread-local mocked LP context.
 *
 * @return A pointer to the mocked logical process context.
 */
struct lp_ctx *test_lp_mock_get(void)
{
	return &lp_mock.lp;
}
