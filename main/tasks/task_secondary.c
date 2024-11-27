//
// Created by samuel on 2024/11/27.
//

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "../state.h"
#include "../utils.h"
#include "../peripherals/display/display.h"
#include "task_secondary.h"

_Noreturn void task_secondary(void* args) {
    printf("Secondary task started on core: %d\n", xPortGetCoreID());
    State* state = args;

    display_init();

    while (1) {
        wdt_feed(CONFIG_ESP_TASK_WDT_TIMEOUT_S * 1000);
        display_update(state);
    }

    vTaskDelete(NULL);
}