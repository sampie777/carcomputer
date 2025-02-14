//
// Created by samuel on 2024/11/27.
//

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "../state.h"
#include "../utils.h"
#include "../peripherals/display/display.h"
#include "task_secondary.h"
#include "../control/data_logger.h"
#include "../connectivity/spi.h"
#include "../peripherals/sd_card.h"
#include "../connectivity/i2c.h"

_Noreturn void task_secondary(void *args) {
    printf("Secondary task started on core: %d\n", xPortGetCoreID());
    State *state = args;

    i2c_init();
    spi_init(state);
    display_init();
    data_logger_init(state);

    list_files_on_sd_card("");
    char *content = NULL;

//    int64_t start_time = esp_timer_get_time_ms();
//    long size = sd_card_read_file("matrix_output.bin", &content);
//    int64_t  stop_time = esp_timer_get_time_ms();
//    printf("Time: %lld ms\n", stop_time - start_time);
    long size = sd_card_get_file_size("matrix_output.bin");

    while (1) {
        wdt_feed(CONFIG_ESP_TASK_WDT_TIMEOUT_S * 1000);
        display_update(state, content, size);
    }

    vTaskDelete(NULL);
}