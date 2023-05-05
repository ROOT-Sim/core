# SPDX-FileCopyrightText: 2008-2023 HPDCS Group <rootsim@googlegroups.com>
# SPDX-License-Identifier: GPL-3.0-only
cmake_minimum_required(VERSION 3.12)
project("ROOT-Sim core" LANGUAGES C DESCRIPTION "A General-Purpose Multi-threaded Parallel/Distributed Simulation Library")

set(PROJECT_VERSION 3.0.0-beta)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_VISIBILITY_PRESET hidden)
# set(CMAKE_UNITY_BUILD ON)
# set(CMAKE_UNITY_BUILD_BATCH_SIZE 128)

include(CheckIPOSupported)
check_ipo_supported(RESULT SUPPORTS_IPO OUTPUT ERROR_IPO)
if(SUPPORTS_IPO)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
    set(CMAKE_C_COMPILE_OPTIONS_IPO "-flto")
else()
    message(WARNING "LTO is not supported")
endif()

if(MSVC)
    message(FATAL_ERROR "You are using the MSVC compiler, which does not properly implement C11. Please consider switching to clang.")
endif()

find_package(Threads REQUIRED)
find_package(Python3 3.6 REQUIRED)
if(NOT DISABLE_MPI)
    find_package(MPI REQUIRED)
endif()

add_compile_options(-Wall -Wextra -pedantic)

include(CheckLibraryExists)
CHECK_LIBRARY_EXISTS(m exp "" HAVE_LIB_M)
if(HAVE_LIB_M)
    set(EXTRA_LIBS m)
else()
    set(EXTRA_LIBS "")
endif()

if(WIN32)
# Needed for some system calls
    set(EXTRA_LIBS ${EXTRA_LIBS} psapi)
endif()

add_subdirectory(src)

# Run the tests
enable_testing()
add_subdirectory(test)

# Generate and inspect documentation
add_subdirectory(docs)
