//
// Created by samuel on 2024/11/27.
//
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "../state.h"
#include "../utils.h"
#include "../connectivity/i2c.h"
#include "../control/control.h"
#include "../connectivity/spi.h"
#include "../control/data_logger.h"
#include "../peripherals/gpsgsm/a9g.h"
#include "task_primary.h"
#include "../peripherals/adc.h"

void init(State *state) {
    state->boot.max_progress = 6;
    state->boot.max_progress += SD_FILE_SEARCH_MIN_POWER;
    state->boot.progress = 0;

    adc_oneshot_init();
    state->boot.progress++;

    i2c_init();
    state->boot.progress++;

    spi_init(state);
    state->boot.progress++;

    control_init(state);
    state->boot.progress++;

    a9g_init(state);
    state->boot.progress++;

    data_logger_init(state);
    state->boot.progress++;

    state->boot.is_booting = false;

    printf("Init done.\n");

    debug_state(state);
}

_Noreturn void task_primary(void *args) {
    printf("Primary task started on core: %d\n", xPortGetCoreID());
    State *state = args;

    init(state);

    int64_t last_time = 0;
    while (1) {
        wdt_feed(CONFIG_ESP_TASK_WDT_TIMEOUT_S * 1000);

        if (esp_timer_get_time_ms() > last_time + 1000) {
            last_time = esp_timer_get_time_ms();
        }

        // Collect data
        control_read_can_bus(state);
        control_read_analog_sensors(state);
        control_read_user_input(state);

        // Process data
        // The next view processes will interfere with the diagnostic activation sequence, as they influence the gas pedal.
        if (state->diagnostics.status == DiagnosticsStep_Off) {
            control_cruise_control(state);
        }
        control_mpu_power(state);
        a9g_process(state);
        control_process_car(state);
        data_logger_process(state);
        control_run_diagnostics_activation(state);
    }

    vTaskDelete(NULL);
}
