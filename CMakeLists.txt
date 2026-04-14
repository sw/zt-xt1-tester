cmake_minimum_required(VERSION 3.10)

project(zt_xt1_tester)

enable_language(C)
enable_language(CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add project sources
list(APPEND SOURCES
    ${CMAKE_SOURCE_DIR}/src/bjt.c
    ${CMAKE_SOURCE_DIR}/src/calib.c
    ${CMAKE_SOURCE_DIR}/src/cap.c
    ${CMAKE_SOURCE_DIR}/src/component.c
    ${CMAKE_SOURCE_DIR}/src/darlington.c
    ${CMAKE_SOURCE_DIR}/src/diode.c
    ${CMAKE_SOURCE_DIR}/src/dmos.c
    ${CMAKE_SOURCE_DIR}/src/emos.c
    ${CMAKE_SOURCE_DIR}/src/globals.c
    ${CMAKE_SOURCE_DIR}/src/igbt.c
    ${CMAKE_SOURCE_DIR}/src/inductor.c
    ${CMAKE_SOURCE_DIR}/src/jfet.c
    ${CMAKE_SOURCE_DIR}/src/main.c
    ${CMAKE_SOURCE_DIR}/src/probe.c
    ${CMAKE_SOURCE_DIR}/src/resistor.c
    ${CMAKE_SOURCE_DIR}/src/thy_triac.c
    ${CMAKE_SOURCE_DIR}/src/tool.c
    ${CMAKE_SOURCE_DIR}/src/uart.c
    ${CMAKE_SOURCE_DIR}/src/ujt.c
    ${CMAKE_SOURCE_DIR}/src/zener.c
)

if(TEST)
    include(CTest)
    add_subdirectory(test)
else()
    set(CMAKE_C_COMPILER /usr/bin/arm-none-eabi-gcc)
    set(CMAKE_CXX_COMPILER /usr/bin/arm-none-eabi-g++)

    include(cmake/toolchain.cmake)

    enable_language(ASM)

    # Custom linker script
    set(TARGET_LD_SCRIPT ${CMAKE_SOURCE_DIR}/ld/n32g031_flash.ld)
    # Custom startup
    # set(TARGET_STARTUP_ASM ${CMAKE_SOURCE_DIR}/startup/startup_n32g031_gcc.s)

    # Add N32G031 SDK
    add_subdirectory(sdk)

    # Target-specific sources not included in test build
    list(APPEND SOURCES
        ${CMAKE_SOURCE_DIR}/src/adc.c
        ${CMAKE_SOURCE_DIR}/src/comp.c
        ${CMAKE_SOURCE_DIR}/src/gpio.c
        ${CMAKE_SOURCE_DIR}/src/helpers.c
        ${CMAKE_SOURCE_DIR}/src/n32g031_it.c
        ${CMAKE_SOURCE_DIR}/src/timer.c
    )
    add_executable(${PROJECT_NAME} ${SOURCES})

    target_link_libraries(${PROJECT_NAME} N32G031_SDK)

    # Generate outputs. Comment out if not needed
    generate_binary_file(${PROJECT_NAME})
    generate_hex_file(${PROJECT_NAME})
    generate_lst_file(${PROJECT_NAME})
    generate_map_file(${PROJECT_NAME})
endif()

target_include_directories(${PROJECT_NAME} PUBLIC src)
