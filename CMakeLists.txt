# SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
# SPDX-License-Identifier: GPL-3.0-only
cmake_minimum_required(VERSION 3.10)
project("ROOT-Sim core" LANGUAGES C DESCRIPTION "A General-Purpose Multi-threaded Parallel/Distributed Simulation Library")

set(PROJECT_VERSION 3.0.0-alpha.3) # todo: use cmake project VERSION argument when we are out of the alpha

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
find_package(MPI REQUIRED)

add_compile_options(-Wall -Wextra -pedantic)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DLOG_LEVEL=LOG_TRACE")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DLOG_LEVEL=LOG_INFO")
add_definitions(-DROOTSIM_VERSION="${PROJECT_VERSION}")

include(CheckLibraryExists)
CHECK_LIBRARY_EXISTS(m exp "" HAVE_LIB_M)
if(HAVE_LIB_M)
    set(EXTRA_LIBS m)
else()
    set(EXTRA_LIBS "")
endif()

if(WIN32)
    set(EXTRA_LIBS ${EXTRA_LIBS} psapi)
endif()

add_subdirectory(src)

# Run the tests
enable_testing()
add_subdirectory(test)

# Generate and inspect documentation
add_subdirectory(docs)
