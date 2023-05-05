# SPDX-FileCopyrightText: 2008-2023 HPDCS Group <rootsim@googlegroups.com>
# SPDX-License-Identifier: GPL-3.0-only
set(rscore_srcs
        arch/io.c
        arch/mem.c
        arch/thread.c
        core/core.c
        init.c
        core/sync.c
        datatypes/msg_queue.c
        distributed/control_msg.c
        gvt/fossil.c
        gvt/gvt.c
        gvt/termination.c
        lib/retractable/retractable.c
        log/file.c
        log/log.c
        log/stats.c
        lp/lp.c
        lp/process.c
        mm/auto_ckpt.c
        mm/buddy/buddy.c
        mm/buddy/ckpt.c
        mm/buddy/multi.c
        mm/msg_allocator.c
        parallel/parallel.c
        serial/serial.c)

if(NOT DISABLE_MPI)
    set(rscore_srcs ${rscore_srcs} distributed/mpi.c)
else()
    set(rscore_srcs ${rscore_srcs} distributed/no_mpi.c)
endif()

# Build the core library
add_library(rscore STATIC ${rscore_srcs})

target_compile_definitions(rscore PRIVATE ROOTSIM_VERSION="${PROJECT_VERSION}")
target_include_directories(rscore PRIVATE .)
target_link_libraries(rscore ${CMAKE_THREAD_LIBS_INIT} ${EXTRA_LIBS})

if(NOT DISABLE_MPI)
    target_include_directories(rscore PRIVATE ${MPI_C_INCLUDE_PATH})
    target_compile_options(rscore PRIVATE ${MPI_C_COMPILE_FLAGS})
    target_link_libraries(rscore ${MPI_C_LIBRARIES})
endif()

install(FILES ROOT-Sim.h DESTINATION include)
install(TARGETS rscore LIBRARY DESTINATION lib)
