# SPDX-FileCopyrightText: 2008-2023 HPDCS Group <rootsim@googlegroups.com>
# SPDX-License-Identifier: GPL-3.0-only

# Retrieve and make available the testing framework
include(FetchContent)
FetchContent_Declare(rstest
        GIT_REPOSITORY https://github.com/ROOT-Sim/testing-framework.git
        GIT_TAG        origin/master
        )
FetchContent_MakeAvailable(rstest)


# Visibility tests
test_program(visibility_weak visibility/visibility_weak.c)
test_program_link_libraries(visibility_weak rscore)
test_program(visibility_override visibility/visibility_override.c)
test_program_link_libraries(visibility_override rscore)

# Test the API
test_program(load core/load.c)
test_program_link_libraries(load rscore)

# Test data structures and subsystems
test_program(bitmap datatypes/bitmap.c)
test_program(mm mm/buddy.c mm/buddy_hard.c mm/parallel.c mm/main.c mock.c)
test_program_link_libraries(mm rscore)
test_program(termination gvt/termination.c)
test_program_link_libraries(termination rscore)

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
test_program(correctness_parallel integration/correctness/parallel.c integration/correctness/application.c integration/correctness/functions.c integration/correctness/output_256.c)
test_program_link_libraries(correctness_parallel rscore)
test_program(phold integration/phold.c)
test_program_link_libraries(phold rscore)

# TODO: The following is garbage and will be removed soon
target_include_directories(test_visibility_weak PRIVATE ${CMAKE_SOURCE_DIR}/src/include)
target_include_directories(test_load PRIVATE ${CMAKE_SOURCE_DIR}/src/include)
target_include_directories(test_mm PRIVATE ${CMAKE_SOURCE_DIR}/src/include)
target_include_directories(test_stats PRIVATE ${CMAKE_SOURCE_DIR}/src/include)
target_include_directories(test_termination PRIVATE ${CMAKE_SOURCE_DIR}/src/include)
target_include_directories(test_sync PRIVATE ${CMAKE_SOURCE_DIR}/src/include)
target_include_directories(test_correctness_serial PRIVATE ${CMAKE_SOURCE_DIR}/src/include)
target_include_directories(test_correctness_parallel PRIVATE ${CMAKE_SOURCE_DIR}/src/include)
target_include_directories(test_phold PRIVATE ${CMAKE_SOURCE_DIR}/src/include)
