#pragma once

#include "pico/stdlib.h"

void led_init(void);
void led_set(bool on);
void led_toggle(void);
uint led_pin(void);
bool led_is_on(void);