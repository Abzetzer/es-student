#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
  RELEASED,
  PREPRESSED,
  PRESSED,
  PRERELEASED,
} button_state_t;

void button_task_init(void);
void button_task_handle(void);

bool button_task_is_pressed();

uint32_t button_task_get_press_count();