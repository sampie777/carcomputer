#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "data.h"
#include "display.h"

State state;

void gui(void* args) {
    while (1) {
        state.car.speed++;
        display_update(&state);
        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

// Running on main core
void app_main(void)
{

    display_init();

    portBASE_TYPE result = xTaskCreatePinnedToCore(&gui, "gui",
                            3584 + 512, NULL,
                                                   0, NULL, 1);

    if (result != pdTRUE) {
        printf("Failed to create GUI task");
    }

    int i = 0;
    while (1) {
        state.car.speed++;
        display_update(&state);
        i++;
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}
