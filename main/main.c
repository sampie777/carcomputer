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

    printf("Init done.\n");
}

void task_process_main(void *args) {
    // Wait to be started by the main task
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    printf("Task process started\n");

    State *state = args;
    int64_t lastQueryTime = 0;
    init(state);

    while (1) {
        if (esp_timer_get_time_ms() > lastQueryTime + 1000) {
            lastQueryTime = esp_timer_get_time_ms();
            printf("Stack: %u/%u; ", uxTaskGetStackHighWaterMark(NULL), MAIN_TASK_STACK_SIZE);
            printf("car: %c; "
                   "can: %c; "
                   "brake: %c; "
                   "reverse: %c; "
                   "speed: %d km/h; "
                   "rpm: %d; "
                   "pedal: %d %%; "
                   "%f / %f V; "
                   "min: %f V; "
                   "max: %f V; "
                   "cc: %c; "
                   "control: %f; "
                   "virtual: %d %%; "
                   "target: %d km/h; "
                   "\n",
                   state->car.is_connected ? 'y' : 'n',
                   state->car.is_controller_connected ? 'y' : 'n',
                   state->car.is_braking ? 'y' : 'n',
                   state->car.is_in_reverse ? 'y' : 'n',
                   (int) state->car.speed,
                   (int) state->car.rpm,
                   (int) (state->car.gas_pedal * 100),
                   state->car.gas_pedal_0_volts,
                   state->car.gas_pedal_1_volts,
                   state->car.gas_pedal_0_min_value_volts,
                   state->car.gas_pedal_0_max_value_volts,
                   state->cruise_control.enabled ? 'y' : 'n',
                   state->cruise_control.control_value,
                   (int) (state->cruise_control.virtual_gas_pedal * 100),
                   (int) state->cruise_control.target_speed
            );
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
    State state = {0};
    state.is_booting = true;
    state.power_off_count_down_sec = -1;;

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
