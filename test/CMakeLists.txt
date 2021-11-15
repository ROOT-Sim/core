# SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
# SPDX-License-Identifier: GPL-3.0-only
include_directories(.)

function(test_program name)
    add_executable(test_${name} ${ARGN})
    target_link_libraries(test_${name} rscore)
    add_test(NAME test_${name} COMMAND test_${name})
    set_tests_properties(test_${name} PROPERTIES TIMEOUT 50)
endfunction()

test_program(load core/load.c)
#test_program(sync core/sync.c)
test_program(numerical lib/numerical.c)
test_program(correctness_serial integration/serial.c integration/model/application.c integration/model/functions.c integration/model/output_256.c)
test_program(correctness_parallel integration/parallel.c integration/model/application.c integration/model/functions.c integration/model/output_256.c)
