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
