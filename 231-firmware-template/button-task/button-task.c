#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include <stdio.h>
#include "button-task.h"
#include "led-task.h"

typedef enum {
  RELEASED,
  PREPRESSED,
  PRESSED,
  PRERELEASED,
} button_state_t;


static button_state_t button_state = RELEASED;
static uint64_t last_toggle_us = 0;
uint32_t button_press_count = 0;
uint8_t BUTTON_PIN = 15;

const uint64_t DEBOUNCE_US = 20000;

void button_task_init(void)
{
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);
}

void button_task_handle(void)
{
    uint32_t button_input = gpio_get(BUTTON_PIN);
    uint64_t now_us = time_us_64();

    switch (button_state)
    {
    // переходы по таблице; при переходе в «нажата»
    case RELEASED:
        if (button_input == 0)
        {
            button_state = PREPRESSED;
            last_toggle_us = now_us;
        }
    break;
    case PREPRESSED:
        if (now_us - last_toggle_us <= DEBOUNCE_US)
        {
            button_state = RELEASED;
        }
        else
        {
            if (button_input == 0)
            {
                button_state = PRESSED;
                button_press_count ++;
                led_task_next_state();
            }
        }
    break;
    case PRESSED:
        if (button_input == 1)
        {
            last_toggle_us = now_us;
            button_state = PREPRESSED;
        }
    break;
    case PRERELEASED:
        if (now_us - last_toggle_us >= DEBOUNCE_US)
        {
            if (button_input == 1)
            {
                button_state = PRESSED;
            }
        }
        else
        {
            button_state = RELEASED;
        }
    // увеличиваем счётчик и вызываем led_task_next_state()
    }
}

bool button_task_is_pressed() 
{
    return button_state == PRESSED; 
}

uint32_t button_task_get_press_count()
{
    return button_press_count;
}