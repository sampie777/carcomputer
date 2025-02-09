//
// Created by samuel on 20-7-22.
//

#ifndef APP_TEMPLATE_GAS_PEDAL_H
#define APP_TEMPLATE_GAS_PEDAL_H

#include "../state.h"
#include <hal/ledc_types.h>

void gas_pedal_enable(uint8_t enable);
int gas_pedal_read(State *state);
void gas_pedal_write(State *state);
void gas_pedal_init(State *state, uint32_t frequency);
double read_pedal_volts(adc1_channel_t channel, int sample_count_factor);
void set_pedal_volts(ledc_channel_t channel, double voltage);

#endif //APP_TEMPLATE_GAS_PEDAL_H
