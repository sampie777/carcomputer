//
// Created by samuel on 28-7-22.
//

#include "buttons.h"
#include "adc.h"

int read_debounced(adc_channel_t channel, uint8_t sample_count,
                   int min_value, unsigned long debounce_cooldown_period,
                   unsigned long min_press_time) {
    return 0;
}

Button getPressedButton0(ButtonsState *state) {
    return BUTTON_NONE;
}

Button getPressedButton1(ButtonsState *state) {
    return BUTTON_NONE;
}

Button buttons_get_pressed(ButtonsState *state, MotionState *motion) {
    return BUTTON_NONE;
}

void buttons_init(){};