#pragma once

#include <stdbool.h>
#include <stdint.h>

void platform_master_init(void);
bool platform_i2c_write(uint8_t address, uint8_t reg, uint8_t value);
bool platform_i2c_read(uint8_t address, uint8_t reg, uint8_t *value);

void platform_led_set(bool on);