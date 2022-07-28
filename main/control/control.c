//
// Created by samuel on 19-7-22.
//

#include "control.h"
#include "../peripherals/canbus.h"
#include "../peripherals/gas_pedal.h"
#include "../return_codes.h"
#include "../peripherals/display/display.h"
#include "cruise_control.h"

void control_read_can_bus(State *state) {
    canbus_check_messages(state);
}

void control_read_analog_sensors(State *state) {
    if (gas_pedal_read(state) == RESULT_DISCONNECTED) {
        display_set_error_message(state, "Pedal disconnected");
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
}
