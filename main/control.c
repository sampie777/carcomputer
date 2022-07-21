//
// Created by samuel on 19-7-22.
//

#include "control.h"
#include "peripherals/canbus.h"
#include "peripherals/gas_pedal.h"
#include "return_codes.h"
#include "peripherals/display.h"

void control_read_can_bus(State *state) {
    canbus_check_messages(state);
    state->car.speed++;
}

void control_read_analog_sensors(State *state) {
    if (gas_pedal_read(&state->car.gas_pedal) == RESULT_DISCONNECTED) {
        display_set_error_message(state, "Pedal disconnected");
    }
}

void control_door_lock(State *state) {}

void control_cruise_control(State *state) {
    gas_pedal_write(state->car.virtual_gas_pedal);
}
