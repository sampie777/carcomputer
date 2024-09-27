//
// Created by samuel on 19-7-22.
//

#include <math.h>
#include "../config.h"
#include "control.h"
#include "../peripherals/canbus/canbus.h"
#include "../return_codes.h"
#include "../peripherals/buttons.h"
#include "../utils.h"
#include "../error_codes.h"
#include "../peripherals/led.h"

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

void control_led_indicator_step(State *state) {
    if (state->is_booting) {
        led_blink(1500);
        return;
    }
    if (!state->car.is_connected) {
        led_blink(800);
        return;
    }
    if (!state->car.gas_pedal_connected) {
        led_blink(300);
        return;
    }

    led_set(state->cruise_control.enabled);
}

void control_cruise_control(State *state) {
#if CRUISE_CONTROL_ENABLE
    cruise_control_step(state);
#endif
}

void control_init(State *state) {
    gpio_set_direction(POWER_PIN, GPIO_MODE_OUTPUT);

    led_init();

    canbus_init(state);
#if CRUISE_CONTROL_ENABLE
    gas_pedal_init(state);
#endif
    buttons_init();
}

CarGearPosition estimate_car_gear(CarState *car) {
    if (car->is_in_reverse) {
        return GearReverse;
    }

    if (car->rpm == 0) {
        return GearNeutral;
    }

    double ratio = (double) car->speed / car->rpm_raw * 10000;
    int rounded_ration = (int) round(ratio);
    switch (rounded_ration) {
        case CAR_GEAR_1_RATIO:
            return Gear1;
        case CAR_GEAR_2_RATIO:
            return Gear2;
        case CAR_GEAR_3_RATIO:
            return Gear3;
        case CAR_GEAR_4_RATIO:
            return Gear4;
        case CAR_GEAR_5_RATIO:
            return Gear5;
        default:
            return GearNeutral;
    }
}

void control_car_gear(State *state) {
    state->car.estimated_gear = estimate_car_gear(&state->car);
}
