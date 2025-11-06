//
// Created by samuel on 20-7-22.
//

#ifndef APP_TEMPLATE_GAS_PEDAL_H
#define APP_TEMPLATE_GAS_PEDAL_H

#include <stdint.h>

#include "../../../../state.h"
#include <hal/ledc_types.h>

void gas_pedal_enable(uint8_t enable);
int gas_pedal_read(State *state);
void gas_pedal_write(State *state);
void gas_pedal_init(State *state, uint32_t frequency);
void set_pedal_volts(ledc_channel_t channel, double voltage);

uint8_t _get_gas_penal_enabled();

#endif //APP_TEMPLATE_GAS_PEDAL_H
