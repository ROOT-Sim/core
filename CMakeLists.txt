cmake_minimum_required(VERSION 3.10)
project("ROOT-Sim core" LANGUAGES C)
set(PROJECT_VERSION 3.0.0-alpha.3)
set(PROJECT_DESCRIPTION "A General-Purpose Multithreaded Parallel/Distributed Simulation Library")
set(CMAKE_VERBOSE_MAKEFILE ON)

add_definitions(-DROOTSIM_VERSION=${PROJECT_VERSION})

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
        src/mm/msg_allocator.c
        src/parallel/parallel.c
        src/serial/serial.c
        )

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_VISIBILITY_PRESET hidden)
#set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

find_package(MPI REQUIRED)
add_compile_options(${TARGET_NAME} -Wall -Wextra -Wpedantic ${MPI_C_COMPILE_FLAGS} -Wno-gnu-zero-variadic-macro-arguments)
#add_link_options(${TARGET_NAME} ${MPI_C_LINK_FLAGS})
link_libraries(${MPI_C_LIBRARIES} pthread)
include_directories(SYSTEM ${MPI_C_INCLUDE_PATH})

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DLOG_LEVEL=LOG_TRACE")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DLOG_LEVEL=LOG_INFO")

# Build the core library
include_directories(src)
add_library(rscore STATIC ${srcs})

# Run the tests
enable_testing()
add_subdirectory(test)

# Generate and inspect documentation
add_subdirectory(docs)
