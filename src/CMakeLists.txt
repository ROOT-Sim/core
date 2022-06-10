# SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
# SPDX-License-Identifier: GPL-3.0-only
set(srcs
        arch/io.c
        arch/mem.c
        arch/thread.c
        core/core.c
        init.c
        core/sync.c
        datatypes/msg_queue.c
        distributed/control_msg.c
        distributed/mpi.c
        gvt/fossil.c
        gvt/gvt.c
        gvt/termination.c
        lib/lib.c
        lib/random/random.c
        lib/state/state.c
        lib/topology/topology.c
        log/file.c
        log/log.c
        log/stats.c
        lp/lp.c
        lp/process.c
        mm/auto_ckpt.c
        mm/buddy/buddy.c
        mm/buddy/ckpt.c
        mm/msg_allocator.c
        parallel/parallel.c
        serial/serial.c)

# Build the core library
add_library(rscore STATIC ${srcs})

target_compile_definitions(rscore PRIVATE ${LOG_LEVEL_DEFINE} PRIVATE ROOTSIM_VERSION="${PROJECT_VERSION}")
target_include_directories(rscore PRIVATE . ${MPI_C_INCLUDE_PATH})
target_compile_options(rscore PRIVATE ${MPI_C_COMPILE_FLAGS})
target_link_libraries(rscore ${MPI_C_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${EXTRA_LIBS})
