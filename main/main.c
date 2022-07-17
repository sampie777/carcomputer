#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

int global = 0;

void gui(void* args) {
    printf("initizizing GUI...\n");
    while (1) {
        global++;
        printf("[ ] Hello world by GUI! Global: %d\n", global);
        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

// Running on main core
void app_main(void)
{

    portBASE_TYPE result = xTaskCreatePinnedToCore(&gui, "gui",
                            3584 + 512, NULL,
                                                   0, NULL, 1);

    if (result != pdTRUE) {
        printf("Failed to create GUI task");
    }

    int i = 0;
    while (1) {
        global++;
        printf("[%d] Hello world! Global: %d\n", i, global);
        i++;
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}
