#pragma once

#include <stdint.h>

#define DEVICE_NAME "es-firmware-template"
#define FIRMWARE_VERSION "1.0.0"

#define DEVICE_PROJECT "231-firmware-template"
#define DEVICE_REPO "https://github.com/<ilya5207>/es-student"

#ifndef DEVICE_BOARD
#define DEVICE_BOARD "unknown"
#endif

void device_info(void);

struct info_t
{
    uint32_t version;
    char name[13];
    uint8_t revision;
};
