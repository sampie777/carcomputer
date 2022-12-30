//
// Created by samuel on 19-7-22.
//

#include <math.h>
#include "control.h"
#include "../peripherals/canbus/canbus.h"
#include "../return_codes.h"
#include "../peripherals/buttons.h"
#include "../utils.h"
#include "../peripherals/mpu9250.h"
#include "../backend/server.h"
#include "../error_codes.h"
#include "../peripherals/gpsgsm/gpsgsm.h"

#if CRUISE_CONTROL_ENABLE
#include "../peripherals/gas_pedal.h"
#include "cruise_control.h"
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
#if CRUISE_CONTROL_ENABLE
    if (gas_pedal_read(state) == RESULT_DISCONNECTED) {
        set_error(state, ERROR_PEDAL_DISCONNECTED);
    }
#endif

    mpu9250_read(state);
}

void control_read_user_input(State *state) {
    static int64_t last_read_time = 0;
    if (esp_timer_get_time_ms() < last_read_time + BUTTONS_READ_INTERVAL_MS) return;
    last_read_time = esp_timer_get_time_ms();

    Button button = buttons_get_pressed();
    switch (button) {
        case BUTTON_NONE:
            break;
        case BUTTON_UP:
            if (state->display.current_screen == Screen_Menu) {
                switch (state->display.menu_option_selection) {
                    case ScreenMenuOption_CruiseControl:
                        state->display.current_screen = Screen_CruiseControl;
                        break;
                    case ScreenMenuOption_Sensors:
                        state->display.current_screen = Screen_Sensors;
                        break;
                    case ScreenMenuOption_GPS:
                        state->display.current_screen = Screen_GPS;
                        break;
                    case ScreenMenuOption_Config:
                        state->display.current_screen = Screen_Config;
                        break;
                    default:
                        break;
                }
            } else if (state->display.current_screen == Screen_CruiseControl) {
                state->cruise_control.enabled = true;
            }
            break;
        case BUTTON_VOLUME_UP:
            if (state->display.current_screen == Screen_Menu) {
                state->display.menu_option_selection++;
                if (state->display.menu_option_selection >= ScreenMenuOption_MAX_VALUE) {
                    state->display.menu_option_selection = 0;
                }
            } else if (state->display.current_screen == Screen_CruiseControl) {
                state->cruise_control.target_speed++;
            }
            break;
        case BUTTON_VOLUME_DOWN:
            if (state->display.current_screen == Screen_Menu) {
                if (state->display.menu_option_selection <= 0) {
                    state->display.menu_option_selection = ScreenMenuOption_MAX_VALUE;
                }
                state->display.menu_option_selection--;
            } else if (state->display.current_screen == Screen_CruiseControl) {
                state->cruise_control.target_speed--;
                if (state->cruise_control.target_speed < 0) {
                    state->cruise_control.target_speed = 0;
                }
            }
            break;
        case BUTTON_SOURCE:
            if (state->display.current_screen == Screen_Menu) {
                state->display.menu_option_selection = 0;
            } if ((!state->cruise_control.enabled && state->display.current_screen == Screen_CruiseControl) ||
                state->display.current_screen == Screen_Sensors ||
                state->display.current_screen == Screen_GPS ||
                state->display.current_screen == Screen_Config ||
                state->display.current_screen == Screen_Registration) {
                state->display.current_screen = Screen_Menu;
            } else if (state->display.current_screen == Screen_CruiseControl) {
                state->cruise_control.enabled = false;
            }
            break;
        case BUTTON_SOURCE_LONG_PRESS:
            utils_reboot(state);
        default:
            break;
    }
}

void control_door_lock(State *state) {
    static uint8_t has_been_locked = false;
    if (state->car.speed < 1) {
        // TODO: This is not going to work, as the doors will try to lock
        //  every time when pulling away (e.g. from traffic light)
        has_been_locked = false;
        return;
    }

    if (state->car.speed < 30) return;

    // Only lock once
    if (has_been_locked) return;
    has_been_locked = true;

    // Lock doors
//    printf("[Control] Locking doors...\n");
//    canbus_send_lock_doors(state, true);  // This CAN message doesn't seem to do anything to the car
}

void control_mpu_power(State *state) {
#if POWER_OFF_ENABLE
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
#endif
}

void control_cruise_control(State *state) {
#if CRUISE_CONTROL_ENABLE
    cruise_control_step(state);
#endif
}

void control_init(State *state) {
    gpio_set_direction(POWER_PIN, GPIO_MODE_OUTPUT);

    canbus_init(state);
#if CRUISE_CONTROL_ENABLE
    gas_pedal_init(state);
#endif
    buttons_init();
    mpu9250_init();
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

    if (state->car.odometer == 0) return;

    // Trip already logged
    if (state->trip_has_been_uploaded || state->car.odometer_start == state->car.odometer) return;

    if (server_send_trip_end(state) != RESULT_OK) {
        // Retry again in X seconds
        engine_off_time = esp_timer_get_time_ms() + TRIP_LOGGER_ENGINE_OFF_GRACE_TIME_MS - TRIP_LOGGER_UPLOAD_RETRY_TIMEOUT_MS;
        return;
    }

    // Track this value in case ignition turns on after trip ended (for closing windows or something).
    state->car.odometer_start = state->car.odometer;
    state->trip_has_been_uploaded = true;
}

void control_crash_detection(State *state) {
    static int64_t last_sent = 0;
    double total_force = sqrt(state->motion.accel_x * state->motion.accel_x + state->motion.accel_y * state->motion.accel_y + state->motion.accel_z * state->motion.accel_z);
    if (total_force < CRASH_DETECTION_CRASH_MIN_G) return;

    if (esp_timer_get_time_ms() < last_sent + CRASH_DETECTION_CRASH_MAX_DURATION_MS) return;
    last_sent = esp_timer_get_time_ms();

#ifdef ICE_CONTACT_NUMBER
    char message[158];   // Max SMS length

    set_error(state, ERROR_CRASH_DETECTED);

    char timestamp[64];
    Time time = state->location.time.year > 2021 ? state->location.time : state->gsm.time;
    if (time.year < 2000) {
        timestamp[0] = '\0';
    } else {
        sprintf(timestamp, "%04d-%02d-%02d'T'%02d:%02d:%02d.000%+d",
                time.year,
                time.month,
                time.day,
                time.hours,
                time.minutes,
                time.seconds,
                time.timezone);
    }

    sprintf(message, "CRASH! Location: %.5f,%.5f at %s (accuracy: %d%%). Force: %.1f g.",
            state->location.latitude,
            state->location.longitude,
            timestamp,
            state->location.satellites / 4 * 100,
            total_force
    );

    // Loop over all specified numbers and send them
    char numbers[] = ICE_CONTACT_NUMBER;
    char *number = strtok(numbers, ";");
    while (number != NULL) {
        gsm_send_sms(number, message);
        number = strtok(NULL, ";");
    }
#else
    set_error(state, ERROR_CRASH_NO_ICE);
#endif
}
