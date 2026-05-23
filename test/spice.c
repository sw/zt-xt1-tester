#include <assert.h>
#include <cmocka.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ngspice/sharedspice.h>
#include "adc.h"
#include "probe.h"
#include "spice.h"
#include "timer.h"

static int spice_command(const char *fmt, ...)
{
    char *p;
    va_list args;
    va_start(args, fmt);
    vasprintf(&p, fmt, args);
    va_end(args);
    // printf("%s; ", p);
    int e = ngSpice_Command(p);
    free(p);
    return e;
}

static bool str_startswith(const char *s, const char *start)
{
    return strncmp(s, start, strlen(start)) == 0;
}

static int send_char(char *s, int id, void *user)
{
    if (str_startswith(s, "stderr Warning:"))
    {
        puts(s);
    }
    assert(!str_startswith(s, "stderr Error:"));
    assert(!str_startswith(s, "stderr simulation aborted"));
    assert(!str_startswith(s, "stderr run simulation(s) aborted"));
}

static int send_stat(char *stat, int id, void *user)
{
    // printf("%s:%s\n", __FUNCTION__, stat);
}

static int controlled_exit(int status, NG_BOOL immediate, NG_BOOL quit, int id, void *user)
{
    if (!quit)
    {
        printf("%s status=%d immediate=%u reason=%s\n",
               __FUNCTION__, status, immediate, quit ? "quit" : "error");
    }
}

static float adc_val[8];
static unsigned int comp_probe;
static char comp_probe_s[3 + 1];
static float comp_threshold;
static bool opamp_enabled;
static double spice_time;
static const double spice_time_max = 10.0;

static int send_data(pvecvaluesall vec_vals, int num, int id, void *user)
{
    for (int i = 0; i < vec_vals->veccount; i++)
    {
        vecvalues *v = vec_vals->vecsa[i];
        // if (strcmp(v->name, comp_probe_s) == 0) { printf("COMP %.3fV\n", v->creal); }
        if (strcmp(v->name, "time") == 0)
        {
            spice_time = v->creal;
            assert(spice_time + 0.1 < spice_time_max); /* .tran stop sufficient? */
        }
        else if (strcmp(v->name, "/a0") == 0)
        {
            //printf("a0 %.3fV\n", v->creal);
            adc_val[1] = v->creal;
        }
        else if (strcmp(v->name, "/a1") == 0)
        {
            //printf("a1 %.3fV\n", v->creal);
            adc_val[3] = v->creal;
        }
        else if (strcmp(v->name, "/a2") == 0)
        {
            //printf("a2 %.3fV\n", v->creal);
            adc_val[7] = v->creal;
        }
        else if (strcmp(v->name, "/az") == 0)
        {
            //printf("az %.3fV\n", v->creal);
            adc_val[6] = opamp_enabled ? v->creal : 0;
        }
    }
}

static int send_init_data(pvecinfoall vec_info, int id, void *user)
{
    // printf("%s\n", __FUNCTION__);
}

static int bg_thread_running(NG_BOOL running, int id, void *user)
{
    printf("%s:%u\n", __FUNCTION__, running);
}

void spice_init(void)
{
    assert(strncmp(NGSPICE_PACKAGE_VERSION, "42", 2) >= 0);
    ngSpice_Init(send_char, send_stat, controlled_exit, send_data, send_init_data, bg_thread_running, NULL);
    spice_command("set ngbehavior=ltpsa");
    spice_command("circbyline .end");
}

void spice_uninit(void)
{
    spice_command("quit");
}

static struct
{
    probe_mode_t direct;
    probe_mode_t r680;
    probe_mode_t r470k;
} probes[3];

static spice_probe_settings_t probe_settings =
{
    .rd = 15.0f,
    .rp = 15.0f,
    .probe12_cap = 26.28f,
    .probe13_cap = 31.97f,
    .probe21_cap = 28.04f,
    .probe23_cap = 31.97f,
    .probe31_cap = 27.93f,
    .probe32_cap = 26.70f,
};

void spice_probe_settings_set(const spice_probe_settings_t *settings)
{
    assert(settings);
    assert(0.0f <= settings->rd);
    assert(settings->rd <= 100.0f);
    assert(0.0f <= settings->rp);
    assert(settings->rp <= 100.0f);
    assert(0.0f <= settings->probe12_cap);
    assert(settings->probe12_cap <= 100.0f);
    assert(0.0f <= settings->probe13_cap);
    assert(settings->probe13_cap <= 100.0f);
    assert(0.0f <= settings->probe21_cap);
    assert(settings->probe21_cap <= 100.0f);
    assert(0.0f <= settings->probe23_cap);
    assert(settings->probe23_cap <= 100.0f);
    assert(0.0f <= settings->probe31_cap);
    assert(settings->probe31_cap <= 100.0f);
    assert(0.0f <= settings->probe32_cap);
    assert(settings->probe32_cap <= 100.0f);
    probe_settings = *settings;
}

void probe_configure(uint_fast8_t probe, probe_mode_t direct, probe_mode_t r680, probe_mode_t r470k)
{
    assert(probe < 3);
    assert(direct < PROBE_MODE_NB);
    assert(r680 < PROBE_MODE_NB);
    assert(r470k < PROBE_MODE_NB);
    probes[probe].direct = direct;
    probes[probe].r680 = r680;
    probes[probe].r470k = r470k;

    spice_command("alter v%u0 = %u", probe, direct == PROBE_DRV_LO ? 5 : 0);
    spice_command("alter v%u1 = %u", probe, direct == PROBE_DRV_HI ? 0 : 5);
    spice_command("alter v%u2 = %u", probe, r680   == PROBE_DRV_LO ? 5 : 0);
    spice_command("alter v%u3 = %u", probe, r680   == PROBE_DRV_HI ? 0 : 5);
    spice_command("alter v%u4 = %u", probe, r470k  == PROBE_DRV_LO ? 5 : 0);
    spice_command("alter v%u5 = %u", probe, r470k  == PROBE_DRV_HI ? 0 : 5);
}

static unsigned int breakpoint_counter;
void tim6_usleep(uint_fast32_t us)
{
    static const char tr[] = { [PROBE_ANALOG] = '~', [PROBE_DRV_LO] = 'v', [PROBE_DRV_HI] = '^' };
    printf("%s(%7lu)     %c %c %c    %c %c %c    %c %c %c\n", __FUNCTION__, us,
        tr[probes[0].direct], tr[probes[0].r680], tr[probes[0].r470k],
        tr[probes[1].direct], tr[probes[1].r680], tr[probes[1].r470k],
        tr[probes[2].direct], tr[probes[2].r680], tr[probes[2].r470k]);

    /*
        Warning: stupidity
        We have to use breakpoints with condition "time >= x" to allow for arbitrary resolution.
        These breakpoints then have to be removed or they will hit again immediately.
        Unfortunately, ngspice numbers them incrementally.
    */
    spice_command("stop when time >= %f", fmax(spice_time + us / 1e6, nextafter(spice_time, INFINITY)));
    spice_command("resume");
    spice_command("delete %u", ++breakpoint_counter);
}

void tim6_msleep(uint_fast32_t ms)
{
    assert(ms < UINT_FAST32_MAX / 1000);
    tim6_usleep(ms * 1000);
}

void spice_dut_set(char **dut, double t_step)
{
    char *circ_a[65];
    int i = 0;

    /* create netlist. all strings allocated on the heap and free'd after parsing */

    circ_a[i++] = strdup(".title Component Tester");

    /* supply rails */
    circ_a[i++] = strdup("v0 +5V 0 dc 5");

    /* probe resistors */
    circ_a[i++] = strdup("r00 /s0 /t0 680");
    circ_a[i++] = strdup("r10 /s1 /t1 680");
    circ_a[i++] = strdup("r20 /s2 /t2 680");
    circ_a[i++] = strdup("r01 /w0 /t0 470k");
    circ_a[i++] = strdup("r11 /w1 /t1 470k");
    circ_a[i++] = strdup("r21 /w2 /t2 470k");

    /* ADC input resistances */
    circ_a[i++] = strdup("r02 /a0 /t0 1500");
    circ_a[i++] = strdup("r12 /a1 /t1 1500");
    circ_a[i++] = strdup("r22 /a2 /t2 1500");
    /* ADC sample and hold capacitors */
    circ_a[i++] = strdup("c00 0 /a0 13p");
    circ_a[i++] = strdup("c10 0 /a1 13p");
    circ_a[i++] = strdup("c20 0 /a2 13p");

    /* GPIO MOSFETs */
    /*
        Other possibly relevant parameters:
            FC - Body diode coefficient for forward-bias depletion capacitance formula
            A  - Non-linear Cgd capacitance parameter
    */
    /* TODO: figure out how to use probe_settings.probeXX_cap */
    asprintf(&circ_a[i++], ".model nmos vdmos nchan "
        "cgdmax=2p "
        "cgdmin=2p "
        "cgs=2p "
        "cjo=2p "
        "rd=%.0f "
        "vto=+1.5"
        , probe_settings.rd);
    asprintf(&circ_a[i++], ".model pmos vdmos pchan "
        "cgdmax=2p "
        "cgdmin=2p "
        "cgs=2p "
        "cjo=2p "
        "rd=%.0f "
        "vto=-1.5"
        , probe_settings.rp);

    circ_a[i++] = strdup("v00 /g00 0 dc 0");
    circ_a[i++] = strdup("v01 /g01 0 dc 5");
    circ_a[i++] = strdup("v02 /g02 0 dc 0");
    circ_a[i++] = strdup("v03 /g03 0 dc 5");
    circ_a[i++] = strdup("v04 /g04 0 dc 0");
    circ_a[i++] = strdup("v05 /g05 0 dc 5");

    circ_a[i++] = strdup("v10 /g10 0 dc 0");
    circ_a[i++] = strdup("v11 /g11 0 dc 5");
    circ_a[i++] = strdup("v12 /g12 0 dc 0");
    circ_a[i++] = strdup("v13 /g13 0 dc 5");
    circ_a[i++] = strdup("v14 /g14 0 dc 0");
    circ_a[i++] = strdup("v15 /g15 0 dc 5");

    circ_a[i++] = strdup("v20 /g20 0 dc 0");
    circ_a[i++] = strdup("v21 /g21 0 dc 5");
    circ_a[i++] = strdup("v22 /g22 0 dc 0");
    circ_a[i++] = strdup("v23 /g23 0 dc 5");
    circ_a[i++] = strdup("v24 /g24 0 dc 0");
    circ_a[i++] = strdup("v25 /g25 0 dc 5");

    circ_a[i++] = strdup("mq00 /t0 /g00  0  nmos");
    circ_a[i++] = strdup("mq01 /t0 /g01 +5v pmos");
    circ_a[i++] = strdup("mq02 /s0 /g02  0  nmos");
    circ_a[i++] = strdup("mq03 /s0 /g03 +5v pmos");
    circ_a[i++] = strdup("mq04 /w0 /g04  0  nmos");
    circ_a[i++] = strdup("mq05 /w0 /g05 +5v pmos");

    circ_a[i++] = strdup("mq10 /t1 /g10  0  nmos");
    circ_a[i++] = strdup("mq11 /t1 /g11 +5v pmos");
    circ_a[i++] = strdup("mq12 /s1 /g12  0  nmos");
    circ_a[i++] = strdup("mq13 /s1 /g13 +5v pmos");
    circ_a[i++] = strdup("mq14 /w1 /g14  0  nmos");
    circ_a[i++] = strdup("mq15 /w1 /g15 +5v pmos");

    circ_a[i++] = strdup("mq20 /t2 /g20  0  nmos");
    circ_a[i++] = strdup("mq21 /t2 /g21 +5v pmos");
    circ_a[i++] = strdup("mq22 /s2 /g22  0  nmos");
    circ_a[i++] = strdup("mq23 /s2 /g23 +5v pmos");
    circ_a[i++] = strdup("mq24 /w2 /g24  0  nmos");
    circ_a[i++] = strdup("mq25 /w2 /g25 +5v pmos");

    /* zener */
    /* TODO: resistor values (and capacitor?) */
    circ_a[i++] = strdup("v30 /vz 0 dc 35.4");
    circ_a[i++] = strdup("i30 /vz /tz dc 3m");
    circ_a[i++] = strdup(".model ideal_diode d(n=0.001)");
    circ_a[i++] = strdup("d30 /tz /vz ideal_diode");
    circ_a[i++] = strdup("r30 /tz /az 68k");
    circ_a[i++] = strdup("r31 /az 0 10k");

    /* DUT */
    for (int d = 0; dut[d]; d++)
    {
        circ_a[i++] = strdup(dut[d]);
    }

    asprintf(&circ_a[i++], ".tran %f %f", t_step, spice_time_max);

    circ_a[i++] = strdup(".end");
    circ_a[i++] = NULL;

    spice_command("destroy all");
    spice_command("remcirc");

    assert(i <= sizeof(circ_a) / sizeof(circ_a[0]));
    int e = ngSpice_Circ(circ_a);
    assert(e == 0);

    while (i--)
    {
        free(circ_a[i]);
    }

    // spice_command("listing deck");
    spice_command("stop when time = 1u");
    spice_command("run");
    spice_command("delete %u", ++breakpoint_counter);
}

uint_fast16_t adc_single(uint_fast8_t channel)
{
    int val = adc_val[channel] * (4096.0f / 5.0f);
    if (val < 0)
    {
        val = 0;
    }
    if (val > 4095)
    {
        val = 4095;
    }
    return val;
}

uint_fast32_t adc_sum(uint_fast8_t channel, uint_fast16_t num)
{
    int_fast64_t val = adc_val[channel] * (4096.0 / 5.0) * num;
    if (val < 0)
    {
        val = 0;
    }
    if (val > 4095 * num)
    {
        val = 4095 * num;
    }
    return val;
}

uint_fast16_t adc_average(uint_fast8_t channel, uint_fast16_t num)
{
    return adc_single(channel);
}

void comp_init(uint_fast8_t probe, uint_fast8_t vref_sel)
{
    assert(probe < 3);
    comp_probe = probe;
    assert(vref_sel < 64);
    comp_threshold = vref_sel * (5.0f / 63.0f); /* TODO: 63 or 64? */
}

uint_fast32_t comp_start(unsigned int probe_pull, unsigned int pullup, uint_fast32_t timeout)
{
    assert(probe_pull < 3);
    assert(pullup < 3);

    snprintf(comp_probe_s, sizeof(comp_probe_s), "/a%u", comp_probe);

    static const char tr[] = { [PROBE_ANALOG] = '~', [PROBE_DRV_LO] = 'v', [PROBE_DRV_HI] = '^' };
    printf("%s(%u, %8lu)  %c %c %c    %c %c %c    %c %c %c\n", __FUNCTION__, pullup, timeout,
        tr[probes[0].direct], tr[probes[0].r680], tr[probes[0].r470k],
        tr[probes[1].direct], tr[probes[1].r680], tr[probes[1].r470k],
        tr[probes[2].direct], tr[probes[2].r680], tr[probes[2].r470k]);

    /* simulate direct write to GPIO PBSC register to pull probe high */
    if (pullup == 0) { probes[probe_pull].direct = PROBE_DRV_HI; }
    if (pullup == 1) { probes[probe_pull].r680   = PROBE_DRV_HI; }
    if (pullup == 2) { probes[probe_pull].r470k  = PROBE_DRV_HI; }
    spice_command("alter v%u%u = 0", probe_pull, pullup * 2);
    spice_command("alter v%u%u = 0", probe_pull, pullup * 2 + 1);

    printf("                         %c %c %c    %c %c %c    %c %c %c\n",
        tr[probes[0].direct], tr[probes[0].r680], tr[probes[0].r470k],
        tr[probes[1].direct], tr[probes[1].r680], tr[probes[1].r470k],
        tr[probes[2].direct], tr[probes[2].r680], tr[probes[2].r470k]);

    double start = spice_time;

    /* add a few ticks to ensure simulation doesn't stop a few ticks earlier than timeout */
    spice_command("stop when time >= %f", spice_time + (timeout + 100) / 48e6);
    spice_command("stop when v(%s) > %f", comp_probe_s, comp_threshold);
    spice_command("resume");
    spice_command("delete %u", ++breakpoint_counter);
    spice_command("delete %u", ++breakpoint_counter);

    comp_probe_s[0] = '\0';
    return (spice_time - start) * 48e6;
}

void opamp_enable(bool enable)
{
    check_expected_uint(enable);
    opamp_enabled = enable;
}
