
#include "api.h"
#include "pico/stdlib.h"
#include "stdio.h"
#include "string.h"
#include "profiling.h"
#include "device.h"
#include "hardware/clocks.h"
#include "log.h"
#include "led-task.h"
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>

// Число без знака из десятичной записи. false — в записи не только цифры
// или число не помещается в uint32_t.
static bool parse_u32(const char *text, uint32_t *value)
{
    // strtoul приняла бы и знак: из "-1" получилось бы 4294967295
    if (!isdigit((unsigned char)text[0]))
    {
        return false;
    }

    char *end;
    errno = 0;
    unsigned long number = strtoul(text, &end, 10);

    // после числа остались символы или число не поместилось
    if (*end != '\0' || errno == ERANGE || number > UINT32_MAX)
    {
        return false;
    }

    *value = (uint32_t)number;
    return true;
}

// обработчик команды: получает имя и аргументы
typedef void (*api_callback_t)(const command_t *command);

// команда прибора: имя, справка для help и обработчик
typedef struct
{
    const char *name;
    const char *help;
    api_callback_t callback;
} api_command_t;

// прикидка: за член ряда 4 операции с double, 175 + 110 + 190 + 110 = 585
// тактов; 1 000 000 членов по 585 тактов при 125 МГц — около 4,7 с
const uint CALC_PI_TERMS = 1000000;

double calc_pi(uint terms) {
  double sum = 0.0;
  double sign = 1.0;

  for (int k = 0; k < terms; k++) {
    // Прибавить очередной член ряда: ±1 / (2k + 1)
    sum += sign / (2.0 * k + 1.0);
    sign = -sign;
  }

  // Сумма ряда равна π/4
  return 4.0 * sum;
}

volatile double pi_result;

void cmd_calc_pi(const command_t *command) {
  uint64_t start_us = time_us_64();
  pi_result = calc_pi(CALC_PI_TERMS);
  uint64_t spent_us = time_us_64() - start_us;

  printf("pi: %.8f\n", pi_result);
  printf("time: %llu ms\n", spent_us / 1000);
}


void cmd_info(const command_t *command) 
{
  device_info();
}

void cmd_uptime(const command_t *command) { printf("uptime: %llu ms\n", time_us_64() / 1000); }

void cmd_main_time_reset(const command_t *command) { main_time_reset(); }

void cmd_main_time_exec(const command_t *command) { main_time_exec(); }

void cmd_help(const command_t *command);

void cmd_led_period(const command_t *command)
{
    uint32_t period_ms;

    if (command->argc != 1 || !parse_u32(command->argv[0], &period_ms))
    {
        printf("error: usage led_period <period_ms>\n");
        return;
    }

    // годится ли такой период, решает задача светодиода
    if (!led_task_set_period_ms(period_ms))
    {
        printf("error: period_ms must be greater than 0\n");
        return;
    }

    // ответ: состояние и период светодиода
    print_led_state();
}

void cmd_led_enable(const command_t *command)
{
    led_task_set_state(LED_STATE_ON);
    print_led_state();
}
void cmd_led_disable(const command_t *command)
{
    led_task_set_state(LED_STATE_OFF);
    print_led_state();
}
void cmd_led_blink(const command_t *command)
{
    led_task_set_state(LED_STATE_BLINK);
    print_led_state();
}

const api_command_t commands[] = {
    {"help", "list of commands", cmd_help},
    {"info", "device passport", cmd_info},
    {"uptime", "time since reset", cmd_uptime},
    {"calc_pi", "calculate pi: calc_pi [terms]", cmd_calc_pi},
    {"main_time_reset", "reset maximum iteration", cmd_main_time_reset},
    {"main_time_exec", "superloop iteration: average and maximum", cmd_main_time_exec},
    {"led_enable", "enable led", cmd_led_enable},
    {"led_disable", "disable led", cmd_led_disable},
    {"led_blink", "blink led", cmd_led_blink},
    {"led_period", "set led period in ms: led_period <period_ms>", cmd_led_period}
};

const uint command_count = sizeof(commands) / sizeof(commands[0]);

void cmd_help(const command_t *command) {
  printf("list of commands:\n");
  for (uint i = 0; i < command_count; i++) {
    printf("  %s: %s\n", commands[i].name, commands[i].help);
  }
}


void api_handle(const command_t *command) 
{
    if (command-> truncated)
    {
        LOG_ERR("error: command longer than 63 characters or 4 arguments\n");
        return;
    }
    for (uint i = 0; i < command_count; i++) {
        if (strcmp(command->name, commands[i].name) == 0) 
        {
        if (commands[i].callback != NULL) 
        {
            commands[i].callback(command);
        }
    
        return;
        }
    }
    
    LOG_ERR("error: unknown command '%s', try help\n", command->name);
}