# SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
# SPDX-License-Identifier: GPL-3.0-only
set(test_srcs
        framework/test.c
        framework/semaphore.c
        framework/thread.c
        framework/rng.c
)

function(test_program name)
    add_executable(test_${name} ${ARGN} ${test_srcs})
    target_link_libraries(test_${name} rscore)
    add_test(NAME test_${name} COMMAND test_${name})
    set_tests_properties(test_${name} PROPERTIES TIMEOUT 60)
endfunction()

# Test framework tests
test_program(testing_framework framework/self-tests/rng.c framework/self-tests/threads.c framework/self-tests/semaphores.c framework/self-tests/main.c)

# Visibility tests
test_program(visibility1 tests/visibility/visibility_weak.c)
#test_program(visibility2 visibility/visibility_override.c)
#test_program(visibility3 visibility/visibility_api.c)

# Test the API
test_program(load tests/core/load.c)

# Test data structures and subsystems
#test_program(msg_queue tests/datatypes/msg_queue.c)
test_program(bitmap tests/datatypes/bitmap.c)
test_program(mm tests/mm/buddy.c tests/mm/buddy_hard.c tests/mm/parallel.c tests/mm/main.c)

# Test libraries
test_program(sync tests/core/sync.c)
test_program(numerical tests/lib/numerical.c)
test_program(topology tests/lib/topology.c)

# Correctness tests
test_program(correctness_serial tests/integration/serial.c tests/integration/model/application.c tests/integration/model/functions.c tests/integration/model/output_256.c)
test_program(correctness_parallel tests/integration/parallel.c tests/integration/model/application.c tests/integration/model/functions.c tests/integration/model/output_256.c)
