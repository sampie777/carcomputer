#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "state.h"
#include "peripherals/display.h"
#include "connectivity/wifi.h"
#include "utils.h"
#include "connectivity/bluetooth.h"

static State state = {0};

void process_gui(void *args) {
    display_init();

    while (1) {
        state.car.speed++;
        printf("Core 2: ");
        display_update(&state);
        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void process_main() {
    bluetooth_init(&state);
    wifi_connect(&state);

    while (1) {
        state.car.speed++;
        printf("Core 1: ");
        display_update(&state);
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

void init() {
    nvs_init();
}

// Running on main core
void app_main(void) {
    init();

    // Init second core
    portBASE_TYPE result = xTaskCreatePinnedToCore(&process_gui, "process_gui",
                                                   3584 + 512, NULL,
                                                   0, NULL, 1);
    if (result != pdTRUE) {
        printf("Failed to create task for second core");
    }

    process_main();
}
