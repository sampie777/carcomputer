//
// Created by samuel on 2024/11/27.
//

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "../config.h"
#include "../state.h"
#include "../peripherals/display/display.h"
#include "task_secondary.h"

_Noreturn void task_secondary(void *args) {
    printf("Secondary task started on core: %d\n", xPortGetCoreID());
    State *state = args;

    display_init();

    TickType_t last_wake_time = xTaskGetTickCount();
    const TickType_t frequency = pdMS_TO_TICKS(DISPLAY_UPDATE_MIN_INTERVAL);

    while (1) {
        display_update(state);
        vTaskDelayUntil(&last_wake_time, frequency);
    }

    vTaskDelete(NULL);
}
