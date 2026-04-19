#pragma once

#include <stdint.h>
#include "component.h"
#include "debug.h"

typedef struct
{
    component_t component;
    uint8_t subtype;
    uint8_t probes[3];
    uint8_t bd;
    uint8_t unknown[6];
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
    union {
        float ic_mA;
        float diode_ir_mA;
    };
    float cap_vloss;
    float temperature;
    float humidity;
    union {
        float diode_vf_a[6];
        struct {
            float misc[2];
            float infrared_f;
        };
    };
    uint8_t infrared_unknown[4];
    uint8_t ds18b20_rom_code[8];
} result_t;
static_assert(sizeof(result_t) == 88);

void main_cycle(void);
