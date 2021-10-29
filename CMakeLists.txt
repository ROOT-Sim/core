cmake_minimum_required(VERSION 3.21.3)
project("ROOT-Sim core" C)
set(CMAKE_VERBOSE_MAKEFILE ON)

add_definitions(-DROOTSIM_VERSION="3.0.0-alpha.3")

set(srcs
        src/arch/io.c
        src/arch/mem.c
        src/arch/thread.c
        src/arch/timer.c
        src/core/core.c
        src/init.c
        src/core/sync.c
        src/datatypes/msg_queue.c
        src/distributed/control_msg.c
        src/distributed/mpi.c
        src/distributed/no_mpi.c
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
        src/mm/mm.c
        src/mm/msg_allocator.c
        src/parallel/parallel.c
        src/serial/serial.c
        )

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_VISIBILITY_PRESET hidden)
#set(CMAKE_UNITY_BUILD ON)
#set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
add_compile_options(${TARGET_NAME} -Wall -Wextra -Wpedantic ${MPI_C_COMPILE_FLAGS})
link_libraries(${MPI_C_LIBRARIES} ${MPI_C_LINK_FLAGS})
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DLOG_LEVEL=LOG_TRACE")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DLOG_LEVEL=LOG_INFO")

find_package(MPI REQUIRED)
include_directories(src)
include_directories(${MPI_INCLUDE_PATH})

add_library(core STATIC ${srcs})
