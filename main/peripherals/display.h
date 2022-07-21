//
// Created by samuel on 17-7-22.
//

#ifndef APP_TEMPLATE_DISPLAY_H
#define APP_TEMPLATE_DISPLAY_H

#include "../state.h"

void display_init();

void display_update(State *state);

void display_set_error_message(State *state, char *message);

#endif //APP_TEMPLATE_DISPLAY_H
