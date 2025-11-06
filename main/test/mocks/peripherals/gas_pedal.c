//
// Created by samuel on 20-7-22.
//

#include "gas_pedal.h"
#include "../../return_codes.h"
#include "adc.h"

void gas_pedal_enable(uint8_t enable) {
}

int is_pedal_connected(double reading_0, double reading_1) {
    return RESULT_OK;
}

double read_pedal_volts(adc_channel_t channel, int sample_count_factor) {
    return RESULT_OK;
}

void read_pedals(State *state, int sample_count_factor) {
}

void set_pedal_volts(ledc_channel_t channel, double voltage) {
}

int gas_pedal_init_minimums(State *state) {
    return RESULT_OK;
}

int gas_pedal_read(State *state) {
    return RESULT_OK;
}

void gas_pedal_write(State *state) {
}

/**
 *
 * @param state App state.
 * @param frequency Frequency for PWM to use. Set to 0 to use max available frequency.
 */
void gas_pedal_init(State *state, uint32_t frequency) {
}
