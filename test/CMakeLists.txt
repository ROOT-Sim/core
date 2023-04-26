# SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
# SPDX-License-Identifier: GPL-3.0-only
add_library(test_framework_lib STATIC
        framework/lp.c
        framework/rng.c
        framework/test.c
        framework/thread.c
        )
target_include_directories(test_framework_lib PRIVATE ../src .)

function(test_program name)
    add_executable(test_${name} ${ARGN})
    target_include_directories(test_${name} PRIVATE ../src .)
    target_link_libraries(test_${name} test_framework_lib rscore) # FIXME this doesn't need to be in all the tests
    add_test(NAME test_${name} COMMAND test_${name})
    set_tests_properties(test_${name} PROPERTIES TIMEOUT 60)
endfunction()

function(test_program_xf name)
    test_program(${name} ${ARGN})
    set_tests_properties(test_${name} PROPERTIES WILL_FAIL TRUE)
endfunction()

# Test framework tests
test_program(self_main framework/self-tests/stubs.c framework/self-tests/main.c)
test_program_xf(self_fail_assert framework/self-tests/stubs.c framework/self-tests/fail_assert.c)
test_program_xf(self_fail_explicit framework/self-tests/stubs.c framework/self-tests/fail_explicit.c)
test_program_xf(self_fail_implicit framework/self-tests/stubs.c framework/self-tests/fail_implicit.c)
test_program_xf(self_fail_implicit_multi framework/self-tests/stubs.c framework/self-tests/fail_implicit_multi.c)
test_program_xf(self_fail_top_level_assert framework/self-tests/stubs.c framework/self-tests/fail_top_level_assert.c)
test_program_xf(self_unexp_pass_assert framework/self-tests/stubs.c framework/self-tests/unexp_pass_assert.c)
test_program_xf(self_unexp_pass_plain framework/self-tests/stubs.c framework/self-tests/unexp_pass_plain.c)

# Visibility tests
test_program(visibility_weak tests/visibility/visibility_weak.c)
test_program(visibility_override tests/visibility/visibility_override.c)

# Test the API
test_program(load tests/core/load.c)

# Test data structures and subsystems
#test_program(msg_queue tests/datatypes/msg_queue.c)
test_program(bitmap tests/datatypes/bitmap.c)
test_program(mm tests/mm/buddy.c tests/mm/buddy_hard.c tests/mm/parallel.c tests/mm/main.c)
test_program(termination tests/gvt/termination.c)

# Test the statistics subsystem
test_program(stats tests/log/stats.c)
add_test(test_stats_parser
        ${Python_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/log/rootsim_stats_test.py
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/log/parse/rootsim_stats.py
        ${CMAKE_CURRENT_BINARY_DIR})

set_tests_properties(test_stats PROPERTIES FIXTURES_SETUP STATISTICS)
set_tests_properties(test_stats_parser PROPERTIES FIXTURES_REQUIRED STATISTICS)

# Test libraries
test_program(sync tests/core/sync.c)

# Integration tests
test_program(correctness_serial tests/integration/correctness/serial.c tests/integration/correctness/application.c tests/integration/correctness/functions.c tests/integration/correctness/output_256.c)
test_program(correctness_parallel tests/integration/correctness/parallel.c tests/integration/correctness/application.c tests/integration/correctness/functions.c tests/integration/correctness/output_256.c)
test_program(phold tests/integration/phold.c)
