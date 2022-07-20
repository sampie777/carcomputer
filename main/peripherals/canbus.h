//
// Created by samuel on 19-7-22.
//

#ifndef APP_TEMPLATE_CANBUS_H
#define APP_TEMPLATE_CANBUS_H

#include "../state.h"

void canbus_init();

void canbus_check_messages(State *state);

#endif //APP_TEMPLATE_CANBUS_H
