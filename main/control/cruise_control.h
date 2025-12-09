//
// Created by samuel on 27-7-22.
//

#ifndef APP_TEMPLATE_CRUISE_CONTROL_H
#define APP_TEMPLATE_CRUISE_CONTROL_H

#include "../state.h"

void cruise_control_step(State *state);
bool cruise_control_safety_checks(State *state, uint8_t car_was_connected);
void cruise_control_config_apply_factor(State *state, int8_t sign);

#endif //APP_TEMPLATE_CRUISE_CONTROL_H
