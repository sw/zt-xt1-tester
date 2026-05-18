#pragma once

#include <stdint.h>

typedef union
{
    struct
    {
        uint8_t id;
        uint8_t counter;
        uint16_t length;
        uint16_t checksum;
        uint8_t payload[88];
    };
    uint8_t raw[94];
} tester_uart_frame_t;
static_assert(sizeof(tester_uart_frame_t) == 94);

typedef enum : uint8_t
{
    COMPONENT_NONE       =  0,
    COMPONENT_TESTING    =  1,
    COMPONENT_JFET       =  2,
    COMPONENT_DMOS       =  3,
    COMPONENT_BJT        =  4,
    COMPONENT_DARLINGTON =  5,
    COMPONENT_UJT        =  6,
    COMPONENT_EMOS       =  7,
    COMPONENT_IGBT       =  8,
    COMPONENT_THYRISTOR  =  9,
    COMPONENT_TRIAC      = 10,
    COMPONENT_DIODE      = 11,
    COMPONENT_2DIODE     = 12,
    COMPONENT_BATTERY    = 13,
    COMPONENT_CAP        = 14,
    COMPONENT_RESISTOR   = 15,
    COMPONENT_INDUCTOR   = 16,
    COMPONENT_ZENER      = 17,
    COMPONENT_INFRARED   = 18,
    COMPONENT_DS18B20    = 19,
    COMPONENT_DHT11      = 20,
} component_t;

typedef enum : uint8_t
{
    JUNCTION_NPN = 1,
    JUNCTION_PNP = 2,
} junction_t;

typedef enum : uint8_t
{
    CHANNEL_N = 1,
    CHANNEL_P = 2,
} channel_t;

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
        float dmos_ugs;
        float jfet_ug;
        float thy_ug;
    };
    float current_mA;
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
} tester_result_t;
static_assert(sizeof(tester_result_t) == 88);

typedef enum : uint8_t
{
    TOOL_NONE           = 0,
    TOOL_RESISTOR       = 1,
    TOOL_INDUCTOR       = 2,
    TOOL_TEMP_DS18B20   = 3,
    TOOL_TEMP_HUM_DHT11 = 4,
    TOOL_INFRARED       = 5,
    TOOL_CALIBRATE      = 6,
} tool_t;
