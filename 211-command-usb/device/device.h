#pragma once
#define DEVICE_NAME "es-led-module"
#define FIRMWARE_VERSION "1.0.0"

#define DEVICE_PROJECT "211-command-usb"
#define DEVICE_REPO "https://github.com/Abzetzer/es-student"

#ifndef DEVICE_BOARD
#define DEVICE_BOARD "unknown"
#endif

#include "stdint.h"

void device_info(void);
void dev_info(void);

struct info_t
{
    uint8_t revision;
    uint32_t version;
    char name[13];
};

extern struct info_t device_card;