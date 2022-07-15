# SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
# SPDX-License-Identifier: GPL-3.0-only
add_library(test_framework_lib STATIC
        framework/lp.c
        framework/rng.c
        framework/semaphore.c
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
test_program(testing_framework framework/self-tests/core.c framework/self-tests/rng.c framework/self-tests/threads.c framework/self-tests/semaphores.c framework/self-tests/main.c)
test_program_xf(testing_framework_fail framework/self-tests/core.c framework/self-tests/fail.c)
test_program_xf(testing_framework_assert1 framework/self-tests/core.c framework/self-tests/fail_assert1.c)
test_program_xf(testing_framework_assert2 framework/self-tests/core.c framework/self-tests/fail_assert2.c)

# Visibility tests
test_program(visibility1 tests/visibility/visibility_weak.c)
test_program(visibility2 tests/visibility/visibility_override.c)

# Test the API
test_program(load tests/core/load.c)

# Test data structures and subsystems
#test_program(msg_queue tests/datatypes/msg_queue.c)
test_program(stats tests/log/stats.c)
test_program(bitmap tests/datatypes/bitmap.c)
test_program(mm tests/mm/buddy.c tests/mm/buddy_hard.c tests/mm/parallel.c tests/mm/main.c)

# Test libraries
test_program(sync tests/core/sync.c)
test_program(numerical tests/lib/numerical.c)
test_program(topology tests/lib/topology.c)

# Integration tests
test_program(correctness_serial tests/integration/correctness/serial.c tests/integration/correctness/application.c tests/integration/correctness/functions.c tests/integration/correctness/output_256.c)
test_program(correctness_parallel tests/integration/correctness/parallel.c tests/integration/correctness/application.c tests/integration/correctness/functions.c tests/integration/correctness/output_256.c)
test_program(phold tests/integration/phold.c)
