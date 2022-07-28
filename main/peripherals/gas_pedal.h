//
// Created by samuel on 20-7-22.
//

#ifndef APP_TEMPLATE_GAS_PEDAL_H
#define APP_TEMPLATE_GAS_PEDAL_H

#include "../state.h"

int gas_pedal_read(State *state);

void gas_pedal_write(State *state);

void gas_pedal_init(State *state);

#endif //APP_TEMPLATE_GAS_PEDAL_H
