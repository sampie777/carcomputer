//
// Created by samuel on 27-7-22.
//

#ifndef APP_TEMPLATE_CRUISE_CONTROL_H
#define APP_TEMPLATE_CRUISE_CONTROL_H

#include "../state.h"

void cruise_control_step(State *state);
double cruise_control_calculate_hour_eta_deviation_for_curren_speed(const State *state);

#endif //APP_TEMPLATE_CRUISE_CONTROL_H
