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
        return;
    }

    if (engine_off_time == 0) {
        engine_off_time = esp_timer_get_time_ms();
    }

    if (esp_timer_get_time_ms() < engine_off_time + TRIP_LOGGER_ENGINE_OFF_GRACE_TIME_MS) return;

    if (state->car.odometer == last_odometer) return;

    // Can't upload data if WiFi is disconnected
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

/**
 * Collect data and write to SD card
 * @param state
 */
void data_logger_log_current(State *state) {
    static int64_t last_log_time = 0;
    if (esp_timer_get_time_ms() < last_log_time + DATA_LOGGER_LOG_INTERVAL) return;
    last_log_time = esp_timer_get_time_ms();

    char buffer[256];
    sprintf(buffer,
            "%lld;"         // esp_timer_get_time_ms()
            "%d;"           // state->car.is_connected
            "%d;"           // state->car.is_controller_connected
            "%d;"           // state->car.is_braking
            "%d;"           // state->car.is_ignition_on
            "%.3f;"         // state->car.speed
            "%.3f;"         // state->car.rpm
            "%d;"           // state->car.odometer
            "%d;"           // state->car.gas_pedal_connected
            "%.5f;"         // state->car.gas_pedal
            "%d;"           // state->cruise_control.enabled
            "%.3f;"         // state->cruise_control.target_speed
            "%.5f;"         // state->cruise_control.virtual_gas_pedal
            "%.5f;"         // state->cruise_control.control_value
            "\"%s\";"       // state->wifi.ssid
            "%d;"           // state->wifi.ip.addr
            "%d;"           // state->wifi.is_connected
            "%d;"           // state->bluetooth.connected
            "%d;"           // state->motion.connected
            "%.3f;"         // state->motion.accel_x
            "%.3f;"         // state->motion.accel_y
            "%.3f;"         // state->motion.accel_z
            "%.3f;"         // state->motion.gyro_x
            "%.3f;"         // state->motion.gyro_y
            "%.3f;"         // state->motion.gyro_z
            "%.3f;"         // state->motion.compass_x
            "%.3f;"         // state->motion.compass_y
            "%.3f;"         // state->motion.compass_z
            "%.3f;"         // state->motion.temperature
            "\n",
            esp_timer_get_time_ms(),
            state->car.is_connected,
            state->car.is_controller_connected,
            state->car.is_braking,
            state->car.is_ignition_on,
            state->car.speed,
            state->car.rpm,
            state->car.odometer,
            state->car.gas_pedal_connected,
            state->car.gas_pedal,
            state->cruise_control.enabled,
            state->cruise_control.target_speed,
            state->cruise_control.virtual_gas_pedal,
            state->cruise_control.control_value,
            state->wifi.ssid,
            state->wifi.ip.addr,
            state->wifi.is_connected,
            state->bluetooth.connected,
            state->motion.connected,
            state->motion.accel_x,
            state->motion.accel_y,
            state->motion.accel_z,
            state->motion.gyro_x,
            state->motion.gyro_y,
            state->motion.gyro_z,
            state->motion.compass_x,
            state->motion.compass_y,
            state->motion.compass_z,
            state->motion.temperature
    );
    sd_card_file_append(state->storage.filename, buffer);
}

void data_logger_process(State *state) {
    data_logger_log_current(state);
    data_logger_upload(state);
}

void data_logger_init(State *state) {
    printf("[DataLogger] Initializing...\n");
    sd_card_init();

    sd_card_create_file_incremental("data", "csv", 0, state->storage.filename);
    sd_card_file_append(state->storage.filename, "timestamp;car_is_connected;car_is_controller_connected;car_is_braking;car_is_ignition_on;car_speed;car_rpm;car_odometer;car_gas_pedal_connected;car_gas_pedal;cruise_control_enabled;cruise_control_target_speed;cruise_control_virtual_gas_pedal;cruise_control_control_value;wifi_ssid;wifi_ip.addr;wifi_is_connected;bluetooth_connected;motion_connected;motion_accel_x;motion_accel_y;motion_accel_z;motion_gyro_x;motion_gyro_y;motion_gyro_z;motion_compass_x;motion_compass_y;motion_compass_z;motion_temperature\n");

    printf("[DataLogger] Init done...\n");
}