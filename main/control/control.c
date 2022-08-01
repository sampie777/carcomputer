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

void control_read_can_bus(State *state) {
    canbus_check_messages(state);
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
    if (++counter < BUTTONS_READ_INTERVAL_LOOPS) {
        return;
    }
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

void control_cruise_control(State *state) {
    cruise_control_step(state);
}

void control_init(State *state) {
    canbus_init(state);
    gas_pedal_init(state);
    buttons_init();
    mpu9250_init();
}
