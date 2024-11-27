//
// Created by samuel on 9-12-22.
//

#ifndef CARCOMPUTER_DISPLAY_SCREENS_H
#define CARCOMPUTER_DISPLAY_SCREENS_H

#include "../../state.h"
#include "sh1106.h"

void content_server_registration(State *state, SH1106Config *sh1106);

void content_cruise_control(State *state, SH1106Config *sh1106);

void content_power_off_count_down(State *state, SH1106Config *sh1106);

void content_motion_sensors_data(const State *state, SH1106Config *sh1106);

void content_location_data(const State *state, SH1106Config *sh1106);

void content_config(const State *state, SH1106Config *sh1106);

void content_main_menu(const State *state, SH1106Config *sh1106);

#endif //CARCOMPUTER_DISPLAY_SCREENS_H
