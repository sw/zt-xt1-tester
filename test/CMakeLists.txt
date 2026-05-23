set(TESTS
    test_bjt_npn.c
    test_bjt_pnp.c
    test_cap.c
    test_darlington_npn.c
    test_darlington_pnp.c
    test_diode.c
    test_dmos.c
    test_emos_n.c
    test_emos_p.c
    test_inductor.c
    test_jfet_n.c
    test_jfet_p.c
    test_none.c
    test_resistor.c
    test_self_adjust.c
    test_thyristor.c
    test_triac.c
    test_ujt.c
    test_zener.c
)

create_test_sourcelist(SOURCES_TEST test_driver.c ${TESTS})

add_executable(${PROJECT_NAME} ${SOURCES} ${SOURCES_TEST} helpers.c spice.c)

target_link_libraries(${PROJECT_NAME} cmocka m ngspice)

foreach(test ${TESTS})
    get_filename_component(name ${test} NAME_WE)
    add_test(NAME ${name} COMMAND ${PROJECT_NAME} ${name})
endforeach()
