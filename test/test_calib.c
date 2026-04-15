#include <cmocka.h>
#include "calib.h"
#include "globals.h"
#include "helpers.h"
#include "main.h"
#include "spice.h"
#include "tool.h"

extern uint8_t uart_received[];

static int setup(void **state)
{
    calib_default();
    spice_init();
    return 0;
}

static int teardown(void **state)
{
    spice_uninit();
    return 0;
}

static void test_calibration(void **state)
{
    /* CALIB_PROBES_CHECK_SHORTED -> start with nothing connected -> stay in CALIB_PROBES_CHECK_SHORTED */
    spice_dut_set((char *[]){NULL}, SPICE_TSTEP_DEFAULT);
    mock_uart(3, 0, 1, (uint8_t[]){TOOL_CALIBRATE});
    main_cycle();
    expect_uint_value(uart_send, id, 8);
    expect_uint_value(uart_send, length, 1);
    mock_second_elapsed();
    main_cycle();
    assert_uint_equal(uart_received[0], CALIB_PROBES_CHECK_SHORTED);

    /* short probes */
    char *shorted[] = {
        "r0 /t0 /t1 1m",
        "r1 /t1 /t2 1m",
        NULL
    };
    spice_dut_set(shorted, SPICE_TSTEP_DEFAULT);
    expect_uint_value(uart_send, id, 8);
    expect_uint_value(uart_send, length, 1);
    mock_second_elapsed();
    main_cycle();
    assert_uint_equal(uart_received[0], CALIB_PROBES_RESISTANCE);

    /* resistance is measured */
    expect_uint_value(uart_send, id, 8);
    expect_uint_value(uart_send, length, 1);
    mock_second_elapsed();
    main_cycle();
    assert_uint_equal(uart_received[0], CALIB_PROBES_CHECK_OPEN);

    /* keep probes shorted -> stay in CALIB_PROBES_CHECK_OPEN */
    expect_uint_value(uart_send, id, 8);
    expect_uint_value(uart_send, length, 1);
    mock_second_elapsed();
    main_cycle();
    assert_uint_equal(uart_received[0], CALIB_PROBES_CHECK_OPEN);

    /* disconnect probes */
    spice_dut_set((char *[]){NULL}, SPICE_TSTEP_DEFAULT);
    expect_uint_value(uart_send, id, 8);
    expect_uint_value(uart_send, length, 1);
    mock_second_elapsed();
    main_cycle();
    assert_uint_equal(uart_received[0], CALIB_PROBES_CAPACITANCE);

    /* capacitance measurement */
    expect_uint_value(uart_send, id, 8);
    expect_uint_value(uart_send, length, 1);
    mock_second_elapsed();
    main_cycle();
    assert_uint_equal(uart_received[0], CALIB_STORE);

    /* store values in flash */
    expect_function_call(calib_write);
    mock_second_elapsed();
    main_cycle();

    assert_float_equal(calibration.rp, 15.0f, 0.1f);
    assert_float_equal(calibration.rd, 15.0f, 0.1f);
    assert_float_in_range(calibration.probe12_cap, 25.2f, 28.7f, 0.0f);
    assert_float_in_range(calibration.probe13_cap, 25.2f, 28.7f, 0.0f);
    assert_float_in_range(calibration.probe21_cap, 25.2f, 28.7f, 0.0f);
    assert_float_in_range(calibration.probe23_cap, 25.2f, 28.7f, 0.0f);
    assert_float_in_range(calibration.probe31_cap, 25.2f, 28.7f, 0.0f);
    assert_float_in_range(calibration.probe32_cap, 25.2f, 28.7f, 0.0f);
}

int test_calib(int argc, char *argv[])
{
    static const struct CMUnitTest tests[] =
    {
        cmocka_unit_test(test_calibration),
    };
    return cmocka_run_group_tests(tests, setup, teardown);
}
