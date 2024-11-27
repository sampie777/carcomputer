#include <driver/adc.h>

#include "config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "state.h"
#include "utils.h"
#include "control/control.h"
#include "connectivity/spi.h"

#define PRIMARY_TASK_STACK_SIZE 32000
#define SECONDARY_TASK_STACK_SIZE 32000

void init(State* state) {
    adc1_config_width(ADC_RESOLUTION - 9);
    spi_init(state);
    control_init(state);

    state->is_booting = false;

    printf("Init done.\n");

    debug_state(state);
}

void task_primary(void* args) {
    printf("Primary task started on core: %d\n", xPortGetCoreID());
    State* state = args;

    init(state);

    int64_t last_time = 0;
    while (1) {
        wdt_feed(CONFIG_ESP_TASK_WDT_TIMEOUT_S * 1000);

        if (esp_timer_get_time_ms() > last_time + 1000) {
            last_time = esp_timer_get_time_ms();

            printf("gas: %lf\t", state->car.gas_pedal);
            printf("%lf / ", state->car.gas_pedal_0_volts);
            printf("%lf V\t", state->car.gas_pedal_1_volts);
            printf("speed: %lf / %lf (%lf %%) ", state->car.speed, state->cruise_control.target_speed, state->cruise_control.control_value);
            printf("\n");
        }

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

void task_secondary(void* args) {
    printf("Secondary task started on core: %d\n", xPortGetCoreID());
    State* state = args;

    while (1) {
        wdt_feed(CONFIG_ESP_TASK_WDT_TIMEOUT_S * 1000);
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

    xTaskCreatePinnedToCore(
        task_primary,
        "task_primary",
        PRIMARY_TASK_STACK_SIZE,
        &state,
        2,
        NULL,
        0);

    xTaskCreatePinnedToCore(
        task_secondary,
        "task_secondary",
        SECONDARY_TASK_STACK_SIZE,
        &state,
        1,
        NULL,
        1);
}
