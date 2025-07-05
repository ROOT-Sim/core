# SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
# SPDX-License-Identifier: GPL-3.0-only

include(test-lib.cmake)

# Visibility tests
test_program(visibility_weak visibility/visibility_weak.c)
test_program_link_libraries(visibility_weak rscore)
target_compile_definitions(test_visibility_weak PRIVATE ROOTSIM_VERSION="${PROJECT_VERSION}")
test_program(visibility_override visibility/visibility_override.c)
test_program_link_libraries(visibility_override rscore)

# Test the API
test_program(load core/load.c)
test_program_link_libraries(load rscore)

# Test data structures and subsystems
test_program(bitmap datatypes/bitmap.c)
test_program(mm mm/buddy.c mm/buddy_hard.c mm/parallel.c mm/main.c mock.c)
target_include_directories(test_mm PRIVATE .)
test_program_link_libraries(mm rscore)
test_program(termination_condition gvt/termination_condition.c)
test_program_link_libraries(termination_condition rscore)
test_program(termination_lp_request gvt/termination_lp_request.c)
test_program_link_libraries(termination_lp_request rscore)
test_program(termination_stop gvt/termination_stop.c)
test_program_link_libraries(termination_stop rscore)

# Test the statistics subsystem
test_program(stats log/stats.c)
add_test(test_stats_parser
        ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/log/rootsim_stats_test.py
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/log/parse/rootsim_stats.py
        ${CMAKE_CURRENT_BINARY_DIR})

set_tests_properties(test_stats PROPERTIES FIXTURES_SETUP STATISTICS)
set_tests_properties(test_stats_parser PROPERTIES FIXTURES_REQUIRED STATISTICS)
test_program_link_libraries(stats rscore)

# Test libraries
test_program(sync core/sync.c)
test_program_link_libraries(sync rscore)

# Integration tests
test_program(correctness_serial integration/correctness/serial.c integration/correctness/application.c integration/correctness/functions.c integration/correctness/output_256.c)
test_program_link_libraries(correctness_serial rscore)
test_program(correctness_parallel integration/correctness/timewarp.c integration/correctness/application.c integration/correctness/functions.c integration/correctness/output_256.c)
test_program_link_libraries(correctness_parallel rscore)
test_program(phold integration/phold.c)
test_program_link_libraries(phold rscore)
