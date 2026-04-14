#pragma once

#include <stddef.h>
#include <stdint.h>
#include "../src/helpers.h"

void mock_uart(uint_fast8_t id, uint_fast8_t counter, size_t length, void *data);
void mock_second_elapsed(void);
void expect_ack(void);
void expect_result(void);
