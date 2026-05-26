#include <cmocka.h>
#include "globals.h"
#include "helpers.h"
#include "main.h"
#include "spice.h"
#include "self_adjust.h"
#include "tool.h"

extern uint8_t uart_received[];

static int setup(void **state)
{
    self_adjust_default();
    spice_init();
    return 0;
}

static int teardown(void **state)
{
    spice_uninit();
    return 0;
}

static void test_self_adjustment(void **state)
{
    /* SELF_ADJUST_PROBES_CHECK_SHORTED -> start with nothing connected -> stay in SELF_ADJUST_PROBES_CHECK_SHORTED */
    spice_dut_set((char *[]){NULL}, SPICE_TSTEP_DEFAULT);
    mock_uart(3, 0, 1, (uint8_t[]){TOOL_SELF_ADJUST});
    main_cycle();
    expect_uint_value(uart_send, id, 8);
    expect_uint_value(uart_send, length, sizeof(self_adjust_state_t));
    mock_second_elapsed();
    main_cycle();
    assert_uint_equal(uart_received[0], SELF_ADJUST_PROBES_CHECK_SHORTED);

    /* short probes */
    char *shorted[] = {
        "r0 /t0 /t1 1m",
        "r1 /t1 /t2 1m",
        NULL
    };
    spice_dut_set(shorted, SPICE_TSTEP_DEFAULT);
    expect_uint_value(uart_send, id, 8);
    expect_uint_value(uart_send, length, sizeof(self_adjust_state_t));
    mock_second_elapsed();
    main_cycle();
    assert_uint_equal(uart_received[0], SELF_ADJUST_PROBES_RESISTANCE);

    /* resistance is measured */
    expect_uint_value(uart_send, id, 8);
    expect_uint_value(uart_send, length, sizeof(self_adjust_state_t));
    mock_second_elapsed();
    main_cycle();
    assert_uint_equal(uart_received[0], SELF_ADJUST_PROBES_CHECK_OPEN);

    /* keep probes shorted -> stay in SELF_ADJUST_PROBES_CHECK_OPEN */
    expect_uint_value(uart_send, id, 8);
    expect_uint_value(uart_send, length, sizeof(self_adjust_state_t));
    mock_second_elapsed();
    main_cycle();
    assert_uint_equal(uart_received[0], SELF_ADJUST_PROBES_CHECK_OPEN);

    /* disconnect probes */
    spice_dut_set((char *[]){NULL}, SPICE_TSTEP_DEFAULT);
    expect_uint_value(uart_send, id, 8);
    expect_uint_value(uart_send, length, sizeof(self_adjust_state_t));
    mock_second_elapsed();
    main_cycle();
    assert_uint_equal(uart_received[0], SELF_ADJUST_PROBES_CAPACITANCE);

    /* capacitance measurement */
    expect_uint_value(uart_send, id, 8);
    expect_uint_value(uart_send, length, sizeof(self_adjust_state_t));
    mock_second_elapsed();
    main_cycle();
    assert_uint_equal(uart_received[0], SELF_ADJUST_STORE);

    /* store values in flash */
    expect_function_call(self_adjust_write);
    mock_second_elapsed();
    main_cycle();

    assert_float_equal(self_adjust_vals.rp, 15.0f, 0.1f);
    assert_float_equal(self_adjust_vals.rd, 15.0f, 0.1f);
    assert_float_in_range(self_adjust_vals.probe12_cap, 24.9f, 29.0f, 0.0f);
    assert_float_in_range(self_adjust_vals.probe13_cap, 24.9f, 29.0f, 0.0f);
    assert_float_in_range(self_adjust_vals.probe21_cap, 24.9f, 29.0f, 0.0f);
    assert_float_in_range(self_adjust_vals.probe23_cap, 24.9f, 29.0f, 0.0f);
    assert_float_in_range(self_adjust_vals.probe31_cap, 24.9f, 29.0f, 0.0f);
    assert_float_in_range(self_adjust_vals.probe32_cap, 24.9f, 29.0f, 0.0f);
}

int test_self_adjust(int argc, char *argv[])
{
    static const struct CMUnitTest tests[] =
    {
        cmocka_unit_test(test_self_adjustment),
    };
    return cmocka_run_group_tests(tests, setup, teardown);
}
