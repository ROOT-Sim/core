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
        log/file.c
        log/log.c
        log/stats.c
        lp/lp.c
        lp/process.c
        lp/expose_lp_state.c
        mm/auto_ckpt.c
        mm/buddy/buddy.c
        mm/buddy/ckpt.c
        mm/buddy/multi.c
        mm/msg_allocator.c
        parallel/parallel.c
        serial/serial.c
        ftl/protocol.c)

if(MPI_FOUND)
    set(rscore_srcs ${rscore_srcs} distributed/mpi.c)
else()
    set(rscore_srcs ${rscore_srcs} distributed/no_mpi.c)
endif()

# Build the core library
add_library(rscore STATIC ${rscore_srcs})

target_compile_definitions(rscore PRIVATE ROOTSIM_VERSION="${PROJECT_VERSION}")
target_include_directories(rscore PRIVATE .)
target_link_libraries(rscore ${CMAKE_THREAD_LIBS_INIT} ${EXTRA_LIBS})

if(MPI_FOUND)
    target_include_directories(rscore PRIVATE ${MPI_C_INCLUDE_PATH})
    target_compile_options(rscore PRIVATE ${MPI_C_COMPILE_FLAGS})
    target_link_libraries(rscore ${MPI_C_LIBRARIES})
endif()

# Add CUDA support
if(CMAKE_CUDA_COMPILER)
    set(rscuda_srcs
            cuda/kernels.cu
            cuda/gpu.cu
#            cuda/nelder_mead.cu
            cuda/nelder_mead_3d.cu
            cuda/queues.cu
            cuda/random.cu
#            cuda/statistics.cu
            cuda/phold/cpu_curand.cu
            cuda/phold/event.cu # This is model stuff!
            cuda/phold/model.cu # This is model stuff!
    )
    set_source_files_properties(${rscuda_srcs} PROPERTIES LANGUAGE CUDA)
    target_sources(rscore PRIVATE ${rscuda_srcs})
    if(APPLE)
        set_property(TARGET rscore PROPERTY BUILD_RPATH ${CMAKE_CUDA_IMPLICIT_LINK_DIRECTORIES})
    endif()
    set_property(TARGET rscore PROPERTY CUDA_RESOLVE_DEVICE_SYMBOLS ON)
    set_property(TARGET rscore PROPERTY CUDA_SEPARABLE_COMPILATION ON)
    add_compile_definitions(HAVE_CUDA)
endif()

install(FILES ROOT-Sim.h DESTINATION include)
install(TARGETS rscore LIBRARY DESTINATION lib)
