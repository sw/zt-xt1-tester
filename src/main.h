#pragma once

#include <stdint.h>
#include "component.h"
#include "debug.h"

typedef struct
{
    component_t component;
    union
    {
        junction_t junction;
        channel_t channel;
    };
    uint8_t probes[3];
    uint8_t bd; /* TODO: does this mean body diode? */
    uint8_t unknown0[6];
    float resistance;
    float capacitance_pF;
    float inductance_uH;
    float diode_vf;
    float hfe;
    union
    {
        float bjt_ube;
        float emos_uth;
        float jfet_ug;
    };
    union
    {
        float ic_mA;
        float diode_ir_mA;
        float ir_0;
    };
    float cap_vloss;
    float temperature;
    float humidity;
    union
    {
        float diode_vf_a[6];
        float ir_a[3];
    };
    uint8_t unknown1[4];
    uint8_t ds18b20_rom_code[8];
} result_t;
static_assert(sizeof(result_t) == 88);

void main_cycle(void);
