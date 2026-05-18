#pragma once

#include <stdbool.h>

void component_do_all(void);

bool bjt(void);

bool cap_bat(void);
bool cap_small(unsigned int p0, unsigned int p1, unsigned int p2, bool subtract_probe);

bool darlington(void);

bool dht11_detect(void);
bool dht11_read(void);

bool diode(void);
void diode_forward_reverse(unsigned int pa, unsigned int pk);

bool dmos(void);

bool ds18b20_detect(void);
bool ds18b20_read(void);

bool emos(void);

bool igbt(void);

bool inductor(void);
bool inductor_tool(void);

bool ir_detect(void);
void ir_read(void);

bool jfet(void);

bool resistor(void);
void resistor_measure(int a, int b, int param);
bool resistor_tool(void);

bool thy_triac(void);

bool ujt(void);

bool zener(void);
