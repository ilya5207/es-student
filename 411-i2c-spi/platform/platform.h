#pragma once

#include <stdbool.h>
#include <stdint.h>

enum {
    PLATFORM_SPI_CS_COUNT = 2,
};

void platform_master_init(void);
bool platform_i2c_write(uint8_t address, uint8_t reg, uint8_t value);
bool platform_i2c_read(uint8_t address, uint8_t reg, uint8_t *value);
void platform_spi_write(uint8_t cs, uint8_t command, uint8_t value);
uint8_t platform_spi_read(uint8_t cs, uint8_t command, uint8_t pause_us);

void platform_start_core1(void (*entry)(void));
typedef uint8_t (*platform_register_read_t)(uint8_t reg);
typedef void (*platform_register_write_t)(uint8_t reg, uint8_t value);

void platform_slave_init(uint8_t address, platform_register_read_t read_register,
                         platform_register_write_t write_register);

void platform_led_set(bool on);

void platform_spi_slave_reset(void);
bool platform_spi_slave_selected(void);
bool platform_spi_slave_receive(uint8_t *byte);
void platform_spi_slave_reply(uint8_t byte);