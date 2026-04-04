set(TESTS
    test_bjt_npn.c
    test_bjt_pnp.c
    test_cap.c
    test_diode.c
    test_emos_n.c
    test_emos_p.c
)

create_test_sourcelist(SOURCES_TEST test_driver.c ${TESTS})

add_executable(${PROJECT_NAME} ${SOURCES} ${SOURCES_TEST} misc.c spice.c)

target_link_libraries(${PROJECT_NAME} m ngspice)

foreach(test ${TESTS})
    get_filename_component(name ${test} NAME_WE)
    add_test(NAME ${name} COMMAND ${PROJECT_NAME} ${name})
endforeach()

set_property(TARGET ${PROJECT_NAME} PROPERTY C_STANDARD 23)
set_property(TARGET ${PROJECT_NAME} PROPERTY C_STANDARD_REQUIRED ON)
