# SPDX-FileCopyrightText: 2008-2025 HPDCS Group <rootsim@googlegroups.com>
# SPDX-License-Identifier: GPL-3.0-only

add_library(rstest STATIC rng.c test.c thread.c)
target_link_libraries(rstest ${CMAKE_THREAD_LIBS_INIT} ${EXTRA_LIBS})
target_include_directories(rstest PUBLIC "${CMAKE_CURRENT_LIST_DIR}")

function(test_program name)
    add_executable(test_${name} ${ARGN})
    target_include_directories(test_${name} PRIVATE ../src)
    target_link_libraries(test_${name} rstest)
    add_test(NAME test_${name} COMMAND test_${name})
    set_tests_properties(test_${name} PROPERTIES TIMEOUT 60)
endfunction()

function(test_program_xf name)
    test_program(${name} ${ARGN})
    set_tests_properties(test_${name} PROPERTIES WILL_FAIL TRUE)
endfunction()

function(test_program_mpi name)
    test_program(${name} ${ARGN})
    set_property(TARGET test_${name} PROPERTY CROSSCOMPILING_EMULATOR "${MPIEXEC_EXECUTABLE};${MPIEXEC_NUMPROC_FLAG};2")
endfunction()

function(test_program_link_libraries name)
    target_link_libraries(test_${name} ${ARGN})
endfunction()

# Self tests for the testing support library
test_program(main self/stubs.c self/main.c)
test_program_xf(fail_assert self/stubs.c self/fail_assert.c)
test_program_xf(fail_explicit self/stubs.c self/fail_explicit.c)
test_program_xf(fail_implicit self/stubs.c self/fail_implicit.c)
test_program_xf(fail_implicit_multi self/stubs.c self/fail_implicit_multi.c)
test_program_xf(fail_top_level_assert self/stubs.c self/fail_top_level_assert.c)
test_program_xf(unexp_pass_assert self/stubs.c self/unexp_pass_assert.c)
test_program_xf(unexp_pass_plain self/stubs.c self/unexp_pass_plain.c)
