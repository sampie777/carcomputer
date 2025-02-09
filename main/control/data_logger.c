//
// Created by samuel on 20-8-22.
//

#include "data_logger.h"
#include <bootloader_random.h>
#include "esp_random.h"
#include "../error_codes.h"
#include "../peripherals/sd_card.h"
#include "../return_codes.h"
#include "../utils.h"

uint32_t generate_session_id() {
    bootloader_random_enable();
    uint32_t id = esp_random();
    bootloader_random_disable();
    return id;
}

void data_logger_init_file(State *state, const char *preferred_file_name) {
    // Check if init is already done
    if (state->storage.filename[0] != 0x00) return;

    char *device_name = state->device_name == NULL || state->device_name[0] == '\0' || state->device_name[0] == 0
                        ? "Default"
                        : state->device_name;

    if (sd_card_create_file_incremental(device_name,
                                        preferred_file_name,
                                        "csv",
                                        state->storage.filename) == RESULT_OVERFLOW) {
        set_error(state, ERROR_SD_FULL);
    }

    printf("[SD] Using file: %s\n", state->storage.filename);

    sd_card_file_append(state->storage.filename,
                        "timestamp;session_id;"
                        "car_is_connected;car_is_controller_connected;car_is_braking;car_is_ignition_on;car_speed;car_rpm;car_odometer;car_gas_pedal_connected;car_gas_pedal;"
                        "cruise_control_enabled;cruise_control_target_speed;cruise_control_virtual_gas_pedal;cruise_control_control_value;"
                        "motion_connected;motion_accel_x;motion_accel_y;motion_accel_z;motion_gyro_x;motion_gyro_y;motion_gyro_z;motion_compass_x;motion_compass_y;motion_compass_z;motion_temperature;"
                        "location_is_gps_on;location_quality;location_satellites;location_is_effective_positioning;location_latitude;location_longitude;location_altitude;location_ground_speed;location_ground_heading;location_datetime;gsm_datetime;"
                        "errors;\n");
}

void data_logger_deinit(State *state) {
    static int64_t engine_off_time = 0;
    static uint32_t start_odometer = 0;

    if (state->power_off_count_down_sec < 0) {
        engine_off_time = 0;

        if (start_odometer == 0) {
            start_odometer = state->car.odometer;
        }
        return;
    }

    if (engine_off_time == 0) {
        engine_off_time = esp_timer_get_time_ms();
    }

    // Give the car chance to start again
    if (esp_timer_get_time_ms() < engine_off_time + DATA_LOGGER_ENGINE_OFF_GRACE_TIME_MS
        && state->power_off_count_down_sec > 0)
        return;

    sd_card_close_file();

    // If the car hasn't moved, proceed to delete the log file
    if (state->car.odometer != start_odometer) return;

    sd_card_delete_file(state->storage.filename);
    state->storage.filename[0] = 0x00;
}

/**
 * Collect data and write to SD card
 * @param state
 */
void data_logger_log_current(State *state) {
    static int64_t last_log_time = 0;

    // Don't log if car isn't on and on the move, so SD card can be swapped safely
    if (!state->car.is_ignition_on && state->car.rpm <= 1) return;

    if (esp_timer_get_time_ms() < last_log_time + DATA_LOGGER_LOG_INTERVAL_MS) return;
    last_log_time = esp_timer_get_time_ms();

    char buffer[256];
    snprintf(buffer, sizeof buffer,
             "%lld;" // esp_timer_get_time_ms()
             "%lu;" // state->logging_session_id
             "%d;" // state->car.is_connected
             "%d;" // state->car.is_controller_connected
             "%d;" // state->car.is_braking
             "%d;" // state->car.is_ignition_on
             "%.3f;" // state->car.speed
             "%.1f;" // state->car.rpm
             "%lu;" // state->car.odometer
             "%d;" // state->car.gas_pedal_connected
             "%.5f;" // state->car.gas_pedal
             "%d;" // state->cruise_control.enabled
             "%.3f;" // state->cruise_control.target_speed
             "%.5f;" // state->cruise_control.virtual_gas_pedal
             "%.5f;" // state->cruise_control.control_value
             "%d;" // state->motion.connected
             "%.3f;" // state->motion.accel_x
             "%.3f;" // state->motion.accel_y
             "%.3f;" // state->motion.accel_z
             "%.3f;" // state->motion.gyro_x
             "%.3f;" // state->motion.gyro_y
             "%.3f;" // state->motion.gyro_z
             "%.3f;" // state->motion.compass_x
             "%.3f;" // state->motion.compass_y
             "%.3f;" // state->motion.compass_z
             "%.3f;" // state->motion.temperature
             "%d;" // state->location.is_gps_on
             "%d;" // state->location.quality
             "%d;" // state->location.satellites
             "%d;" // state->location.is_effective_positioning
             "%.5f;" // state->location.latitude
             "%.5f;" // state->location.longitude
             "%.1f;" // state->location.altitude
             "%.3f;" // state->location.ground_speed
             "%.2f;" // state->location.ground_heading
             "%04d-%02d-%02d'T'%02d:%02d:%02d.000%+d;" // state->location.time
             "%04d-%02d-%02d'T'%02d:%02d:%02d.000%+d;" // state->gsm.time
             "%lu;" // state->errors
             "\n",
             esp_timer_get_time_ms(),
             state->logging_session_id,
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
             state->motion.temperature,
             state->location.is_gps_on,
             state->location.quality,
             state->location.satellites,
             state->location.is_effective_positioning,
             state->location.latitude,
             state->location.longitude,
             state->location.altitude,
             state->location.ground_speed,
             state->location.ground_heading,
             state->location.time.year,
             state->location.time.month,
             state->location.time.day,
             state->location.time.hours,
             state->location.time.minutes,
             state->location.time.seconds,
             state->location.time.timezone,
             state->gsm.time.year,
             state->gsm.time.month,
             state->gsm.time.day,
             state->gsm.time.hours,
             state->gsm.time.minutes,
             state->gsm.time.seconds,
             state->gsm.time.timezone,
             state->errors
    );

    if (sd_card_file_append(state->storage.filename, buffer) == RESULT_OK) {
        state->storage.is_connected = true;
    } else {
        state->storage.is_connected = false;
    }
}

void data_logger_manage_file_name(State *state) {
    static bool has_set_time_as_file_name = false;

    if (state->storage.filename[0] == 0x00) {
        has_set_time_as_file_name = false;
        data_logger_init_file(state, "trip");
    }

    if (has_set_time_as_file_name) return;

    // The next operations are time-consuming, so don't do them when high priority tasks are running
    if (state->cruise_control.enabled) return;

    Time time = state->location.time.year > 2021 ? state->location.time : state->gsm.time;
    if (time.year < 2000) return;

    char timestamp[64];
    snprintf(timestamp, sizeof timestamp, "%04d-%02d-%02dT%02d%02d%02d",
             time.year,
             time.month,
             time.day,
             time.hours,
             time.minutes,
             time.seconds);

    char old_name[strlen(state->storage.filename) + 1];
    strcpy(old_name, state->storage.filename);

    // Create new file name
    data_logger_init_file(state, timestamp);
    printf("[DataLogger] Renaming file from %s to %s\n", old_name, state->storage.filename);

    // Rename old file to new file
    sd_card_delete_file(state->storage.filename);
    sd_card_rename_file(old_name, state->storage.filename);
    has_set_time_as_file_name = true;
}

void data_logger_process(State *state) {
    static int64_t last_init_time = 0;

    data_logger_deinit(state);
    // Don't proceed if we are powering down
    if (state->power_off_count_down_sec >= 0) return;

    if (state->storage.is_connected == false) {
        if (esp_timer_get_time_ms() > last_init_time + 3000) {
            data_logger_init(state);
            last_init_time = esp_timer_get_time_ms();
        }
    }

    data_logger_manage_file_name(state);

    data_logger_log_current(state);
}

void data_logger_init(State *state) {
    printf("[DataLogger] Initializing...\n");

    while (state->logging_session_id == 0) {
        state->logging_session_id = generate_session_id();
    }

    if (sd_card_init() != RESULT_OK) {
        state->storage.is_connected = false;
        printf("[DataLogger] Init failed\n");
        return;
    }
    state->storage.is_connected = true;

    data_logger_init_file(state, "trip");

    printf("[DataLogger] Init done\n");
}
