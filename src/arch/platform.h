/**
 * @file arch/platform.h
 *
 * @brief Determine on what OS we are compiling
 *
 * This header performs some macro checks set by common compilers to determine
 * if we are compiling towards a POSIX target or a Windows target.
 * In case of POSIX, it also tries to set some macros used to understand what
 * is the POSIX version on which we are running.
 * It also tries to set some macros which exploit C11 attributes, if available.
 *
 * @copyright
 * Copyright (C) 2008-2020 HPDCS Group
 * https://rootsim.github.io/core
 *
 * This file is part of ROOT-Sim (ROme OpTimistic Simulator).
 *
 * ROOT-Sim is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; only version 3 of the License applies.
 *
 * ROOT-Sim is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ROOT-Sim; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#pragma once

#if defined(_WIN32)
#define __WINDOWS
#elif defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
#define __POSIX

#if defined(__APPLE__) && defined(__MACH__)
#define __MACOS
#endif

#else
#error Unsupported operating system
#endif

/* Activate some POSIX functionality (e.g. clock_gettime and recursive mutexes) */
#if defined(__POSIX)
#undef _FEATURES_H
#if !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif
#if !defined(_POSIX_C_SOURCE) || ((_POSIX_C_SOURCE - 0) < 199309L)
#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L
#endif
#if !defined(_XOPEN_SOURCE) || ((_XOPEN_SOURCE - 0) < 500)
#undef _XOPEN_SOURCE
#define _XOPEN_SOURCE 500
#endif
#define _XPG6
#endif

/* Compiler-specific information */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define TTHREAD_NORETURN _Noreturn
#elif defined(__GNUC__)
#define TTHREAD_NORETURN __attribute__((__noreturn__))
#else
#define TTHREAD_NORETURN
#endif


/**
* @def _Thread_local
* Thread local storage keyword.
* A variable that is declared with the @c _Thread_local keyword makes the
* value of the variable local to each thread (known as thread-local storage,
* or TLS). Example usage:
* @code
* // This variable is local to each thread.
* _Thread_local int variable;
* @endcode
* @note The @c _Thread_local keyword is a macro that maps to the corresponding
* compiler directive (e.g. @c __declspec(thread)).
* @note This directive is currently not supported on Mac OS X (it will give
* a compiler error), since compile-time TLS is not supported in the Mac OS X
* executable format. Also, some older versions of MinGW (before GCC 4.x) do
* not support this directive, nor does the Tiny C Compiler.
* @hideinitializer
*/

#if !(defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201102L)) && !defined(_Thread_local)
#if defined(__GNUC__) || defined(__INTEL_COMPILER) || defined(__SUNPRO_CC) || defined(__IBMCPP__)
#define _Thread_local __thread
#else
#define _Thread_local __declspec(thread)
#endif
#elif defined(__GNUC__) && defined(__GNUC_MINOR__) && (((__GNUC__ << 8) | __GNUC_MINOR__) < ((4 << 8) | 9))
#define _Thread_local __thread
#endif
