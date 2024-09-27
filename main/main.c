#include <driver/adc.h>
#include "config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "state.h"
#include "utils.h"
#include "control/control.h"
#include "connectivity/spi.h"

#define MAIN_TASK_STACK_SIZE 32000

void init(State *state) {
    adc1_config_width(ADC_RESOLUTION - 9);
    spi_init(state);
    control_init(state);

    state->is_booting = false;
}

void task_process_main(void *args) {
    // Wait to be started by the main task
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    printf("Task process started\n");

    State *state = args;
    int64_t lastQueryTime = 5000;

    init(state);

    while (1) {
        if (esp_timer_get_time_ms() > lastQueryTime + 1000) {
            lastQueryTime = esp_timer_get_time_ms();
            printf("Stack size: %u / %u\n", uxTaskGetStackHighWaterMark(NULL), MAIN_TASK_STACK_SIZE);
        }

        // Collect data
        control_read_can_bus(state);
        control_read_analog_sensors(state);
        control_read_user_input(state);

        // Process data
        control_cruise_control(state);
        control_car_gear(state);
        control_led_indicator_step(state);

        vTaskDelay(10);
    }

    vTaskDelete(NULL);
}

// Running on main core
void app_main(void) {
    State state = {
        .is_booting = true,
        .power_off_count_down_sec = -1,
    };

    printf("Init done.\n");

    TaskHandle_t task_process_main_handle;
    BaseType_t result = xTaskCreate(
        task_process_main,
        "task_process_main",
        MAIN_TASK_STACK_SIZE,
        &state,
        1,
        &task_process_main_handle);

    if (result != pdPASS) printf("Task creation failed: %d.\n", result);
    xTaskNotifyGive(task_process_main_handle);
}
