/**
 * @file core/compiler.h
 *
 * @brief Compiler version detection variable
 *
 * Compiler version detection variable
 *
 * The variable rootsim_compiler_version is used to detect the compiler version.
 * It is set to 0 by default, but it is overridden by the compiler when a model is compiled.
 * The macro ROOTSIM_COMPILER_VERSION is used to check the compiler version.
 *
 * SPDX-FileCopyrightText: 2008-2023 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <stdint.h>

/// The version of the compiler used to generate a model
uint32_t __attribute__((weak)) rootsim_compiler_version = 0;

/// This macro allows to convert a major, minor and patch version into a single uint32_t
#define ROOTSIM_COMPILER_VERSION(a,b,c) (uint32_t)(((a) << 16) + ((b) << 8) + (c))
