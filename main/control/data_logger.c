//
// Created by samuel on 20-8-22.
//

#include "data_logger.h"
#include "../peripherals/sd_card.h"
#include "../return_codes.h"
#include "../utils.h"
#if WIFI_ENABLE
#include "../connectivity/server.h"
#endif

void data_logger_upload(State *state) {
    static int64_t engine_off_time = 0;
    static uint32_t last_odometer = 0;

    if (state->car.is_ignition_on) {
        engine_off_time = 0;
        state->trip_has_been_uploaded = false;
        return;
    }

    if (engine_off_time == 0) {
        engine_off_time = esp_timer_get_time_ms();
    }

    if (esp_timer_get_time_ms() < engine_off_time + TRIP_LOGGER_ENGINE_OFF_GRACE_TIME_MS) return;

    if (state->car.odometer == last_odometer) return;

    // Can't log trip if WiFi is disconnected
    if (!state->wifi.is_connected) return;

#if WIFI_ENABLE
    if (server_send_data_log_record(state) != RESULT_OK) {
        // Retry again in X seconds
        engine_off_time = esp_timer_get_time_ms() + TRIP_LOGGER_ENGINE_OFF_GRACE_TIME_MS - TRIP_LOGGER_UPLOAD_RETRY_TIMEOUT_MS;
        return;
    }
#endif

    last_odometer = state->car.odometer;
}

void data_logger_log_current(State *state) {
    // Collect data and write to SD card

}

void data_logger_process(State *state) {
    data_logger_log_current(state);
    data_logger_upload(state);
}

void data_logger_init() {
    sd_card_init();

    sd_card_test();
    sd_card_deinit();
}