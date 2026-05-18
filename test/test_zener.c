#include <cmocka.h>
#include "calib.h"
#include "helpers.h"
#include "interface.h"
#include "main.h"
#include "spice.h"

extern uint8_t uart_received[];
static const tester_result_t *const result_p = (tester_result_t *)uart_received;

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

static void test_1n5221(void **state)
{
    char *dut[3] =
    {
        ".include \"../../../test/spice/1N5221.txt\"",
        "xd1 0 /tz di_1n5221b",
        NULL
    };
    spice_dut_set(dut, SPICE_TSTEP_DEFAULT);

    expect_ack();
    expect_uint_value(opamp_enable, enable, true);
    expect_uint_value(opamp_enable, enable, false);
    expect_result();
    mock_uart(1, 0, 1, (uint8_t[]){1});
    main_cycle();

    assert_uint_equal(result_p->component, COMPONENT_ZENER);
    assert_float_equal(result_p->diode_vf, 2.18f, 0.01f);
}

static void test_1n4751(void **state)
{
    char *dut[3] =
    {
        ".include \"../../../test/spice/1N4751.txt\"",
        "xd1 0 /tz di_1n4751a",
        NULL
    };
    spice_dut_set(dut, SPICE_TSTEP_DEFAULT);

    expect_ack();
    expect_uint_value(opamp_enable, enable, true);
    expect_uint_value(opamp_enable, enable, false);
    expect_result();
    mock_uart(1, 0, 1, (uint8_t[]){1});
    main_cycle();

    assert_uint_equal(result_p->component, COMPONENT_ZENER);
    assert_float_equal(result_p->diode_vf, 29.8f, 0.1f);
}

int test_zener(int argc, char *argv[])
{
    static const struct CMUnitTest tests[] =
    {
        cmocka_unit_test(test_1n5221),
        cmocka_unit_test(test_1n4751),
    };
    return cmocka_run_group_tests(tests, setup, teardown);
}
