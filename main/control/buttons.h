//
// Created by samuel on 2024/12/06.
//

#ifndef CONTROL_BUTTONS_H
#define CONTROL_BUTTONS_H

#include "../state.h"
#include "../peripherals/buttons.h"

void control_buttons_handle(State* state, Button button);

void control_buttons_handle_pid_config(State* state, Button button);

#endif //CONTROL_BUTTONS_H
