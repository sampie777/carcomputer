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
#include "../peripherals/gpsgsm/a9g.h"
#include "task_primary.h"

void init(State* state) {
    adc1_config_width(ADC_RESOLUTION - 9);
    i2c_init();
    spi_init(state);
    control_init(state);
    a9g_init(state);
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

            debug_print_message_log();

            printf("%d:%02d:%02d  %d-%d-%04d; ",
                    state->location.time.hours,
                    state->location.time.minutes,
                    state->location.time.seconds,
                    state->location.time.day,
                    state->location.time.month,
                    state->location.time.year
            );

            printf("%.5lf, %.5lf; ", state->location.latitude, state->location.longitude);
            printf("Q:%d S:%d E:%d A:%.0lf; ",
                    state->location.quality,
                    state->location.satellites,
                    state->location.is_effective_positioning,
                    state->location.altitude);
            printf("%6.2lf km/h @ %6.1lf*", state->location.ground_speed, state->location.ground_heading);
            printf("\n");
        }

        // Collect data
        control_read_can_bus(state);
        control_read_analog_sensors(state);
        control_read_user_input(state);

        // Process data
        // The next view processes will interfere with the error reading sequence, as they influence the gas pedal.
        if (state->error_codes.status == ErrorCodes_Off) {
            control_cruise_control(state);
        }
        control_mpu_power(state);
        a9g_process(state);
        control_car_gear(state);
        control_led_indicator_step(state);
        data_logger_process(state);
        control_read_error_codes(state);
    }

    vTaskDelete(NULL);
}
