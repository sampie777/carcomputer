//
// Created by samuel on 19-7-22.
//

#include "control.h"
#include "../peripherals/canbus.h"
#include "../peripherals/gas_pedal.h"
#include "../return_codes.h"
#include "../peripherals/display/display.h"
#include "cruise_control.h"
#include "../peripherals/buttons.h"
#include "../utils.h"
#include "../peripherals/mpu9250.h"
#include "../peripherals/sd_card.h"

#if WIFI_ENABLE
#include "../connectivity/server.h"
#endif

void control_read_can_bus(State *state) {
    canbus_check_controller_connection(state);
    canbus_check_messages(state);

    if (esp_timer_get_time_ms() < state->car.last_can_message_time + CAR_CAN_MAX_MESSAGE_RECEIVE_TIMEOUT) {
        state->car.is_connected = true;
    } else {
        state->car.is_connected = false;
    }
}

void control_read_analog_sensors(State *state) {
    if (gas_pedal_read(state) == RESULT_DISCONNECTED) {
        display_set_error_message(state, "Pedal disconnected");
    }

    mpu9250_read(state);
}

void control_read_user_input(State *state) {
    // Only check input once every X loop cycles
    static int counter = 0;
    if (++counter < BUTTONS_READ_INTERVAL_LOOPS) return;
    counter = 0;

    Button button = buttons_get_pressed();
    switch (button) {
        case BUTTON_NONE:
            break;
        case BUTTON_UP:
            state->cruise_control.enabled = true;
            break;
        case BUTTON_VOLUME_UP:
            state->cruise_control.target_speed++;
            break;
        case BUTTON_VOLUME_DOWN:
            state->cruise_control.target_speed--;
            if (state->cruise_control.target_speed < 0) {
                state->cruise_control.target_speed = 0;
            }
            break;
        case BUTTON_SOURCE:
            state->cruise_control.enabled = false;
            break;
        case BUTTON_SOURCE_LONG_PRESS:
            utils_reboot(state);
        default:
            break;
    }
}

void control_door_lock(State *state) {
    static uint8_t has_been_locked = false;
    if (state->car.speed < 30) {
        has_been_locked = false;
        return;
    }

    // Only lock once
    if (has_been_locked) return;
    has_been_locked = true;

    // Lock doors
    // ...
}

void control_mpu_power(State *state) {
    static int64_t ignition_off_time = 0;
    if (state->car.is_ignition_on) {
        gpio_set_level(POWER_PIN, 1);
        ignition_off_time = 0;
        state->power_off_count_down_sec = -1;
        return;
    }

    if (ignition_off_time == 0) {
        ignition_off_time = esp_timer_get_time_ms();
    }

    state->cruise_control.enabled = false;
    long remaining_ms = (long) (ignition_off_time + POWER_OFF_MAX_TIMEOUT_MS - esp_timer_get_time_ms());
    state->power_off_count_down_sec = (int16_t) (remaining_ms / 1000);

    if (state->trip_has_been_uploaded && remaining_ms > 3000) {
        // Set countdown to 3 sec.
        ignition_off_time = esp_timer_get_time_ms() - POWER_OFF_MAX_TIMEOUT_MS + 3000;
    }

    if (esp_timer_get_time_ms() < ignition_off_time + POWER_OFF_MAX_TIMEOUT_MS) return;

    gpio_set_level(POWER_PIN, 0);
    delay_ms(1000);
    ignition_off_time = 0;
}

void control_cruise_control(State *state) {
    cruise_control_step(state);
}

void control_init(State *state) {
    gpio_set_direction(POWER_PIN, GPIO_MODE_OUTPUT);

    canbus_init(state);
    gas_pedal_init(state);
    buttons_init();
    mpu9250_init();
    sd_card_init();
}

void control_trip_logger(State *state) {
    static int64_t engine_off_time = 0;

    if (state->car.is_ignition_on) {
        engine_off_time = 0;
        state->trip_has_been_uploaded = false;
        return;
    }

    if (engine_off_time == 0) {
        engine_off_time = esp_timer_get_time_ms();
    }

    if (esp_timer_get_time_ms() < engine_off_time + TRIP_LOGGER_ENGINE_OFF_GRACE_TIME_MS) return;

    if (state->car.odometer_end == 0) return;

    // Can't log trip if WiFi is disconnected
    if (!state->wifi.is_connected) return;

    // Trip already logged
    if (state->trip_has_been_uploaded || state->car.odometer_start == state->car.odometer_end) return;

#if WIFI_ENABLE
    if (server_send_trip_end(state) != RESULT_OK) {
        // Retry again in X seconds
        engine_off_time = esp_timer_get_time_ms() + TRIP_LOGGER_ENGINE_OFF_GRACE_TIME_MS - TRIP_LOGGER_UPLOAD_RETRY_TIMEOUT_MS;
        return;
    }
#endif

    // Track this value in case ignition turns on after trip ended (for closing windows or something).
    state->car.odometer_start = state->car.odometer_end;
    state->trip_has_been_uploaded = true;
}
