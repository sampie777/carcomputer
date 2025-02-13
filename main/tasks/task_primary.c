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

        if (esp_timer_get_time_ms() > last_time + 5000) {
            last_time = esp_timer_get_time_ms();

//            state->display.current_screen++;
//            if (state->display.current_screen > Screen_AboutCar) {
//                state->display.current_screen = 0;
//            }
//
////            debug_print_message_log();
//
//            printf(""
//                   "[%c] initialized; \n"
//                   "[%c] network_attached; \n"
//                   "[%c] pnp_parameters_set; \n"
//                   "[%c] pnp_activated; \n"
//                   "[%c] agps_enabled; \n"
//                   "[%c] gps_enabled; \n"
//                   "[%c] gps_logging_enabled; \n"
//                   "[%c] gps_logging_started"
//                   "",
//                   state->a9g.initialized ? 'Y' : 'N',
//                   state->a9g.network_attached ? 'Y' : 'N',
//                   state->a9g.pnp_parameters_set ? 'Y' : 'N',
//                   state->a9g.pnp_activated ? 'Y' : 'N',
//                   state->a9g.agps_enabled ? 'Y' : 'N',
//                   state->a9g.gps_enabled ? 'Y' : 'N',
//                   state->a9g.gps_logging_enabled ? 'Y' : 'N',
//                   state->a9g.gps_logging_started ? 'Y' : 'N'
//            );
//            printf("\n");
//
//            printf("%d:%02d:%02d  %d-%d-%04d; ",
//                   state->location.time.hours,
//                   state->location.time.minutes,
//                   state->location.time.seconds,
//                   state->location.time.day,
//                   state->location.time.month,
//                   state->location.time.year
//            );
//
//            printf("%.5lf, %.5lf; ", state->location.latitude, state->location.longitude);
//            printf("Q:%d S:%d E:%d A:%.0lf; ",
//                   state->location.quality,
//                   state->location.satellites,
//                   state->location.is_effective_positioning,
//                   state->location.altitude);
//            printf("%6.2lf km/h @ %6.1lf*", state->location.ground_speed, state->location.ground_heading);
//            printf("\n");
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
