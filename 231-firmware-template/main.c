#include "log.h"
#include "device.h"
#include "led-task.h"
#include "button-task.h"
#include "pi-task.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "profiling.h"
#include "stdio-text-protocol.h"
#include "api.h"

typedef enum
{
    LOOP
} profiling_result_main_t;



int main()
{
    profiling_init();
    led_task_init();
    pi_task_init();
    button_task_init();
    stdio_text_protocol_init();

    profiling_stopwatch_init(STOPWATCH_LOOP, "loop");
    profiling_stopwatch_init(STOPWATCH_COMMAND, "command");
    profiling_stopwatch_init(STOPWATCH_LED, "led");
    profiling_stopwatch_init(STOPWATCH_BUTTON, "button");
    profiling_stopwatch_init(STOPWATCH_PI, "pi");

    LOG_INF("%s %s\n", DEVICE_NAME, FIRMWARE_VERSION);

    while (1)
    {
        profiling_start(STOPWATCH_LED);
        led_task_handle();
        profiling_stop(STOPWATCH_LED);

        profiling_start(STOPWATCH_BUTTON);
        button_task_handle();
        profiling_stop(STOPWATCH_BUTTON);

        profiling_start(STOPWATCH_LOOP);

        profiling_start(STOPWATCH_PI);
        pi_task_handle();
        profiling_stop(STOPWATCH_PI);

        profiling_start(STOPWATCH_COMMAND);
        // принятая строка становится командой, команда — действием прибора
        const command_t *command = stdio_text_protocol_handle();
        if (command != NULL)
        {
            api_handle(command);
        }
        profiling_stop(STOPWATCH_COMMAND);

        profiling_stop(STOPWATCH_LOOP);
    }
}