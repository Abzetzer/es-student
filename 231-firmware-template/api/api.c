
#include "api.h"
#include "pico/stdlib.h"
#include "stdio.h"
#include "string.h"
#include "profiling.h"
#include "device.h"
#include "hardware/clocks.h"
#include "log.h"
#include "led-task.h"
#include "button-task.h"
#include "pi-task.h"
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
volatile double pi_result;

void cmd_pi_start(const command_t *command)
{
    uint32_t terms = 1000000;
    if (command->argc > 0)
    {
        parse_u32(command->argv[0], &terms);
    }

    if (terms <= 0)
    {
        printf("error: usage calc_pi [terms]\n");
        return;
    }

    if (pi_task_start(terms))
    {
        printf("pi: started, %u terms\n", (unsigned int)terms);
    }
    else
    {
        printf("error: pi is busy\n");
    }
}

void cmd_pi(const command_t *command)
{
    pi_task_result_t result = pi_task_get_result();
    switch (result.state)
    {
    case PI_STATE_NOT_STARTED:
        printf("pi: not_started\n");
        break;

    case PI_STATE_PROCESS:
        printf("pi: runing\n");
        break;

    case PI_STATE_FINISH:
        printf("pi: %.8f in %llu ms\n", result.pi, result.time / 1000);
        break;
    }
}

void cmd_info(const command_t *command) 
{
    device_info_t info;
    device_get_info(&info);

    printf("project: %s\n", info.project);
    printf("repo: %s\n", info.repo);
    printf("board: %s\n", info.board);
    printf("serial: %s\n", info.serial);
    printf(
        "chip: manufacturer 0x%03x, part 0x%04x, revision %u\n",
        (unsigned int)info.chip_manufacturer,
        (unsigned int)info.chip_part,
        (unsigned int)info.chip_revision);
    printf("pico-sdk: %s\n", info.pico_sdk);
}

void cmd_uptime(const command_t *command) { printf("uptime: %llu ms\n", time_us_64() / 1000); }

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

void cmd_button(const command_t *command)
{
    printf("button: %s, presses %u\n", button_task_is_pressed() ? "pressed" : "released", button_task_get_press_count());
}

void cmd_profiling(const command_t *command)
{

    for (uint32_t id = 0; id < STOPWATCH_COUNT; ++id)
    {
        profiling_result_t result;
        profiling_get(id, &result);
        printf(
            "%-18s\tcount %u\tmean %f us\tmax %u us\n",
            result.name,
            (unsigned int)result.n,
            result.avg_us,
            (unsigned int)result.max_us);
    }
}

void cmd_profiling_reset(const command_t *command)
{
    profiling_reset();
}

const api_command_t commands[] = {
    {"help", "list of commands", cmd_help},
    {"info", "device passport", cmd_info},
    {"uptime", "time since reset", cmd_uptime},
    {"led_enable", "enable led", cmd_led_enable},
    {"led_disable", "disable led", cmd_led_disable},
    {"led_blink", "blink led", cmd_led_blink},
    {"led_period", "set led period in ms: led_period <period_ms>", cmd_led_period},
    {"button", "show button state and press count", cmd_button},
    {"pi_start", "start calculating pi number", cmd_pi_start},
    {"pi", "result or current status", cmd_pi},
    {"profiling", "profiling", cmd_profiling},
    {"profiling_reset", "profiling reset", cmd_profiling_reset},
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