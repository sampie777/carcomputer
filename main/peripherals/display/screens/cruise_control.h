//
// Created by Samuel-Anton Jansen on 2025/11/19.
//

#ifndef CARCOMPUTER_CRUISE_CONTROL_H
#define CARCOMPUTER_CRUISE_CONTROL_H

#include "../../../state.h"
#include "../sh1106.h"

void content_cruise_control(State *state, SH1106Config *display);
void content_cruise_control_graph(State *state, SH1106Config *display);
void content_cruise_control_eta(State *state, SH1106Config *display);
void content_pedal_control(State *state, SH1106Config *display);

#endif //CARCOMPUTER_CRUISE_CONTROL_H