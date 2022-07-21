//
// Created by samuel on 19-7-22.
//

#ifndef APP_TEMPLATE_CONTROL_H
#define APP_TEMPLATE_CONTROL_H

#include "state.h"

void control_read_can_bus(State *state);

void control_read_analog_sensors(State *state);

void control_door_lock(State *state);

void control_cruise_control(State *state);

#endif //APP_TEMPLATE_CONTROL_H
