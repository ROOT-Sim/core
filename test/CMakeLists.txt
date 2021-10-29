include_directories(.)

function(test_program name)
    add_executable(test_${name} ${ARGN})
    target_link_libraries(test_${name} core m)
    add_test(NAME test_${name} COMMAND test_${name})
    set_tests_properties(test_${name} PROPERTIES TIMEOUT 50)
endfunction()

test_program(load core/load.c)
test_program(numerical lib/numerical.c)
