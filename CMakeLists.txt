# SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
# SPDX-License-Identifier: GPL-3.0-only
cmake_minimum_required(VERSION 3.10)
project("ROOT-Sim core" LANGUAGES C)
set(PROJECT_VERSION 3.0.0-alpha.3)
set(PROJECT_DESCRIPTION "A General-Purpose Multithreaded Parallel/Distributed Simulation Library")

#set(CMAKE_VERBOSE_MAKEFILE ON)

add_definitions(-DROOTSIM_VERSION="${PROJECT_VERSION}")

set(srcs
        src/arch/io.c
        src/arch/mem.c
        src/arch/thread.c
        src/core/core.c
        src/init.c
        src/core/sync.c
        src/datatypes/msg_queue.c
        src/distributed/control_msg.c
        src/distributed/mpi.c
        src/gvt/fossil.c
        src/gvt/gvt.c
        src/gvt/termination.c
        src/lib/lib.c
        src/lib/random/random.c
        src/lib/state/state.c
        src/lib/topology/topology.c
        src/log/log.c
        src/log/stats.c
        src/lp/lp.c
        src/lp/process.c
        src/mm/auto_ckpt.c
        src/mm/buddy/buddy.c
        src/mm/buddy/ckpt.c
        src/mm/msg_allocator.c
        src/parallel/parallel.c
        src/serial/serial.c)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_VISIBILITY_PRESET hidden)
#set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
#set(CMAKE_UNITY_BUILD ON)
#set(CMAKE_UNITY_BUILD_BATCH_SIZE 128)

find_package(Threads REQUIRED)
find_package(MPI REQUIRED)

if(MSVC)
    message(FATAL_ERROR "You are using the MSVC compiler, which does not properly implement C11. Please consider switching to clang.")
endif()

add_compile_options(${TARGET_NAME} ${MPI_C_COMPILE_FLAGS})
link_libraries(${MPI_C_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
if(NOT WIN32)
    link_libraries(m)
else()
    link_libraries(psapi)
endif()
include_directories(SYSTEM ${MPI_C_INCLUDE_PATH})

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DLOG_LEVEL=LOG_TRACE")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DLOG_LEVEL=LOG_INFO")

# Build the core library
include_directories(src)
add_library(rscore STATIC ${srcs})

# Run the tests
enable_testing()
include_directories(test)
add_subdirectory(test)

# Generate and inspect documentation
add_subdirectory(docs)
