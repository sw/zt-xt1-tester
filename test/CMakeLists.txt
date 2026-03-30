set(TESTS
    test_foo.c
)

create_test_sourcelist(SOURCES_TEST test_driver.c ${TESTS})

add_executable(${PROJECT_NAME} ${SOURCES} ${SOURCES_TEST})

foreach(test ${TESTS})
    get_filename_component(name ${test} NAME_WE)
    add_test(NAME ${name} COMMAND ${PROJECT_NAME} ${name})
endforeach()
