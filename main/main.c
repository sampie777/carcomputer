#include <driver/adc.h>

#include "config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "state.h"
#include "utils.h"
#include "control/control.h"
#include "connectivity/spi.h"

#define MAIN_TASK_STACK_SIZE 32000

void init(State* state) {
    adc1_config_width(ADC_RESOLUTION - 9);
    spi_init(state);
    control_init(state);

    state->is_booting = false;

    printf("Init done.\n");
}

void task_process_main(void* args) {
    State* state = args;

    // Wait to be started by the main task
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    printf("Task process started\n");

    init(state);

    debug_state(state);

    while (1) {
        wdt_feed(CONFIG_ESP_TASK_WDT_TIMEOUT_S * 1000);

        // Collect data
        control_read_can_bus(state);
        control_read_analog_sensors(state);
        control_read_user_input(state);

        // Process data
        control_cruise_control(state);
        control_car_gear(state);
        control_led_indicator_step(state);
    }

    vTaskDelete(NULL);
}

// Running on main core
void app_main(void) {
    static State state = {0};
    state.is_booting = true;
    state.power_off_count_down_sec = -1;
    state.cruise_control.pidKp = CRUISE_CONTROL_PID_Kp;
    state.cruise_control.pidKi = CRUISE_CONTROL_PID_Ki;
    state.cruise_control.pidKd = CRUISE_CONTROL_PID_Kd;

    TaskHandle_t task_main_handle;
    BaseType_t result = xTaskCreate(
        task_process_main,
        "task_main",
        MAIN_TASK_STACK_SIZE,
        &state,
        1,
        &task_main_handle);

    if (result != pdPASS) printf("Task creation failed: %d.\n", result);
    xTaskNotifyGive(task_main_handle);
}
