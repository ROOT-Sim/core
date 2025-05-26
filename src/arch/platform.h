/**
 * @file arch/platform.h
 *
 * @brief Determine on what OS we are compiling
 *
 * This header performs some macro checks set by common compilers to determine
 * if we are compiling towards a POSIX target or a Windows target.
 * In case of POSIX, it also tries to set some macros used to understand what
 * is the POSIX version on which we are running.
 *
 * SPDX-FileCopyrightText: 2008-2025 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#if defined(_WIN32)
#define __WINDOWS
// windows specific :)))
#define _CRT_SECURE_NO_WARNINGS
#define _pure
#elif defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
#define __POSIX
// todo get rid of this: right now it is needed for pthread_setaffinity_np()
#define _GNU_SOURCE
#define _pure __attribute__((pure))

#if defined(__linux__)
#define __LINUX
#endif

#if defined(__APPLE__) && defined(__MACH__)
#define __MACOS
#endif

#else
#error Unsupported operating system
#endif
