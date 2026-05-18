#include <cmocka.h>
#include <stdlib.h>
#include "calib.h"
#include "helpers.h"
#include "interface.h"
#include "main.h"
#include "spice.h"

extern uint8_t uart_received[];
static const tester_result_t *const result_p = (tester_result_t *)uart_received;

static const unsigned int probes[][3] =
{
    {0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}
};

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

static void test_one(void **state)
{
    intptr_t i = (intptr_t)*state;
    char *dut[3] = { ".include \"../../../test/spice/DN2530.txt\"" };
    /*                    drain gate source */
    asprintf(&dut[1], "mq1 /t%u /t%u /t%u dn2530", probes[i][1], probes[i][0], probes[i][2]);
    spice_dut_set(dut, SPICE_TSTEP_DEFAULT);
    free(dut[1]);

    expect_ack();
    expect_result();
    mock_uart(1, 0, 1, (uint8_t[]){0});
    main_cycle();

    assert_uint_equal(result_p->component, COMPONENT_DMOS);
    assert_uint_equal(result_p->channel, CHANNEL_N);
    assert_float_equal(result_p->resistance, 5.65f, 0.01f);
    assert_float_equal(result_p->emos_uth, 1.51f, 0.01f);
    /* TODO: check gate capacitance, seems rather high */
    assert_float_in_range(result_p->capacitance_pF, 252.0, 266.0f, 0.0f);
    assert_uint_equal(result_p->probes[0], probes[i][0]);
    assert_uint_equal(result_p->probes[1], probes[i][1]);
    assert_uint_equal(result_p->probes[2], probes[i][2]);
}

int test_dmos(int argc, char *argv[])
{
    struct CMUnitTest tests[sizeof(probes) / sizeof(probes[0])];
    for (intptr_t i = 0; i < sizeof(probes) / sizeof(probes[0]); i++)
    {
        tests[i] = (struct CMUnitTest)cmocka_unit_test_prestate(test_one, (void *)i);
    }
    return cmocka_run_group_tests(tests, setup, teardown);
}
