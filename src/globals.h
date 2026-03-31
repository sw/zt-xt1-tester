#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include "component.h"
#include "tool.h"

typedef struct
{
    uint32_t magic0;
    float probe12_cap;
    float probe13_cap;
    float probe21_cap;
    float probe23_cap;
    float probe31_cap;
    float probe32_cap;
    float rp;
    float rd;
    uint32_t magic1;
} calibration_t;
static_assert(sizeof(calibration_t) == 40);

typedef struct
{
    uint8_t id;
    uint8_t counter;
    uint8_t unused[4];  /* length / checksum ? */
    uint8_t test_type;
    uint8_t unknown[127];
} uart_frame_rx_t;
static_assert(sizeof(uart_frame_rx_t) == 134);

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
} uart_frame_tx_t;
static_assert(sizeof(uart_frame_tx_t) == 94);

typedef struct
{
    component_t component;
    uint8_t subtype;
    uint8_t probes[3];
    uint8_t bd;
    uint8_t unknown[6];
    float resistance;
    float capacitance;
    float inductance;
    float vdiff;
    float hfe;
    float ube;
    float ic_mA;
    float cap_vloss;
    float temperature;
    float humidity;
    float misc[2];
    float infrared_f;
    uint8_t infrared_unknown[16];
    uint32_t temp_hum[2];
} result_t;
static_assert(sizeof(result_t) == 88);

extern calibration_t calibration;
extern uart_frame_rx_t uart_frame_rx;
extern uart_frame_tx_t uart_frame_tx;
volatile extern uint32_t uart_rx_len;
volatile extern bool uart_rx_pending;

extern tool_t tool;

extern result_t result;

extern volatile uint32_t mainloop_seconds;
extern volatile uint32_t mainloop_centiseconds;
