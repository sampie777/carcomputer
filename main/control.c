//
// Created by samuel on 19-7-22.
//

#include "control.h"
#include "peripherals/canbus.h"

void control_read_can_bus(State *state) {
    canbus_check_messages(state);
    state->car.speed++;
}

void control_door_lock(State *state) {}

void control_cruise_control(State *state) {}
