//
// Created by samuel on 27-7-22.
//

#ifndef APP_TEMPLATE_CRUISE_CONTROL_H
#define APP_TEMPLATE_CRUISE_CONTROL_H

#include "../state.h"

void cruise_control_step(State *state);
bool cruise_control_safety_checks(State *state, uint8_t car_was_connected);

#endif //APP_TEMPLATE_CRUISE_CONTROL_H
