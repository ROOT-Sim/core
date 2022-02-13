# SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
# SPDX-License-Identifier: GPL-3.0-only
include_directories(.)

set(test_srcs
        test.c
        semaphore.c
        thread.c)

function(test_program name)
    add_executable(test_${name} ${ARGN} ${test_srcs})
    target_link_libraries(test_${name} rscore)
    add_test(NAME test_${name} COMMAND test_${name})
    set_tests_properties(test_${name} PROPERTIES TIMEOUT 60)
endfunction()

# Test framework tests
test_program(threads self/threads.c)
test_program(semaphores self/semaphores.c)

# Visibility tests
test_program(visibility1 visibility/visibility_weak.c)
test_program(visibility2 visibility/visibility_override.c)
test_program(visibility3 visibility/visibility_api.c)

# Test the API
test_program(load core/load.c)

# Test data structures
test_program(msg_queue datatypes/msg_queue.c)
test_program(bitmap datatypes/bitmap.c)
test_program(buddy mm/buddy.c)

# Test libraries
test_program(sync core/sync.c)
test_program(numerical lib/numerical.c)

# Correctness tests
test_program(correctness_serial integration/serial.c integration/model/application.c integration/model/functions.c integration/model/output_256.c)
test_program(correctness_parallel integration/parallel.c integration/model/application.c integration/model/functions.c integration/model/output_256.c)
