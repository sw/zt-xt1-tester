#include <cmocka.h>
#include "helpers.h"
#include "interface.h"
#include "main.h"
#include "self_adjust.h"
#include "spice.h"

extern uint8_t uart_received[];
static const tester_result_t *const result_p = (tester_result_t *)uart_received;

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

static void test_no_component(void **state)
{
    char *dut[1] = { NULL };
    spice_dut_set(dut, SPICE_TSTEP_DEFAULT);

    expect_ack();
    expect_result();
    mock_uart(1, 0, 1, (uint8_t[]){0});
    main_cycle();

    if (result_p->component != COMPONENT_NONE)
    {
        assert_uint_equal(result_p->component, COMPONENT_CAP);
        assert_float_in_range(result_p->capacitance_pF, 0.0f, 2.0f, 0.0f);
    }
}

int test_none(int argc, char *argv[])
{
    static const struct CMUnitTest tests[] =
    {
        cmocka_unit_test(test_no_component),
    };
    return cmocka_run_group_tests(tests, setup, teardown);
}
