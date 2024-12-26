//
// Created by samuel on 2024/11/27.
//
#include <driver/adc.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "../config.h"
#include "../state.h"
#include "../utils.h"
#include "../connectivity/i2c.h"
#include "../control/control.h"
#include "../connectivity/spi.h"
#include "../control/data_logger.h"
#include "task_primary.h"

void init(State* state) {
    adc1_config_width(ADC_RESOLUTION - 9);
    i2c_init();
    spi_init(state);
    control_init(state);
    data_logger_init(state);

    state->is_booting = false;

    printf("Init done.\n");

    debug_state(state);
}

_Noreturn void task_primary(void* args) {
    printf("Primary task started on core: %d\n", xPortGetCoreID());
    State* state = args;

    init(state);

    int64_t last_time = 0;
    while (1) {
        wdt_feed(CONFIG_ESP_TASK_WDT_TIMEOUT_S * 1000);

        if (esp_timer_get_time_ms() > last_time + 1000) {
            last_time = esp_timer_get_time_ms();

            printf("ign: %c\t", state->car.is_ignition_on ? 'Y' : 'N');
            printf("seatbelt: %c\t", state->car.is_seatbelt_on ? 'Y' : 'N');
            printf("parking brake: %c\t", state->car.is_parking_brake_on ? 'Y' : 'N');
            printf("\n");
        }

        // Collect data
        control_read_can_bus(state);
        control_read_analog_sensors(state);
        control_read_user_input(state);

        // Process data
        control_mpu_power(state);
        control_cruise_control(state);
        control_car_gear(state);
        control_led_indicator_step(state);
        data_logger_process(state);
    }

    vTaskDelete(NULL);
}
