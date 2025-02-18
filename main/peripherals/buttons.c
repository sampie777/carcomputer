//
// Created by samuel on 28-7-22.
//

#include "buttons.h"
#include "../utils.h"
#include "adc.h"

int read_debounced(adc_channel_t channel, uint8_t sample_count,
                   int min_value, unsigned long debounce_cooldown_period,
                   unsigned long min_press_time) {
    static int64_t last_action_time = 0;

    // Debounce button using cooldown period
    if (esp_timer_get_time_ms() < last_action_time + debounce_cooldown_period)
        return -1;

    // Debounce button using minimum pressed time
    int64_t pressStartTime = esp_timer_get_time_ms();
    double last_reading = adc_average_read_channel(channel, sample_count);
    double max_reading = last_reading;
    while (last_reading > min_value && esp_timer_get_time_ms() < pressStartTime + min_press_time) {
        delay_ms(10);
        last_reading = adc_average_read_channel(channel, sample_count);
        max_reading = max(max_reading, last_reading);
    }

    if (max_reading < min_value || esp_timer_get_time_ms() < pressStartTime + min_press_time)
        return 0;

    last_action_time = esp_timer_get_time_ms();
    return (int) max_reading;
}

Button getPressedButton0(ButtonsState *state) {
    static Button previous_button = BUTTON_NONE;
    static int64_t press_start_time = 0;

    state->button0 = read_debounced(BUTTONS_ADC_CHANNEL_0, BUTTON_AVERAGE_READ_SAMPLES, BUTTON_LOWER_LIMIT,
                                    BUTTON_DEBOUNCE_COOLDOWN_PERIOD_MS, BUTTON_MIN_PRESS_TIME_MS);

    Button button = BUTTON_NONE;
    if (state->button0 == -1) {
        button = previous_button;
    } else if (state->button0 > BUTTON_UPPER_LIMIT) {
        button = BUTTON_SOURCE;
    } else if (state->button0 > BUTTON_MIDDLE_LIMIT) {
        button = BUTTON_UP;
    } else if (state->button0 > BUTTON_LOWER_LIMIT) {
        button = BUTTON_VOLUME_UP;
    }

    if (button != previous_button &&
        previous_button != BUTTON_SOURCE_LONG_PRESS &&
        previous_button != BUTTON_UP_LONG_PRESS &&
        previous_button != BUTTON_VOLUME_UP_LONG_PRESS) {
        press_start_time = esp_timer_get_time_ms();
    }

    if (esp_timer_get_time_ms() > press_start_time + BUTTON_LONG_PRESS_MS) {
        if (button == BUTTON_SOURCE) {
            button = BUTTON_SOURCE_LONG_PRESS;
        } else if (button == BUTTON_UP) {
            button = BUTTON_UP_LONG_PRESS;
        } else if (button == BUTTON_VOLUME_UP) {
            button = BUTTON_VOLUME_UP_LONG_PRESS;
        }
    }

    previous_button = button;
    return button;
}

Button getPressedButton1(ButtonsState *state) {
    static Button previous_button = BUTTON_NONE;
    static int64_t press_start_time = 0;

    state->button1 = read_debounced(BUTTONS_ADC_CHANNEL_1, BUTTON_AVERAGE_READ_SAMPLES, BUTTON_LOWER_LIMIT,
                                    BUTTON_DEBOUNCE_COOLDOWN_PERIOD_MS, BUTTON_MIN_PRESS_TIME_MS);

    Button button = BUTTON_NONE;
    if (state->button1 == -1) {
        button = previous_button;
    } else if (state->button1 > BUTTON_UPPER_LIMIT) {
        button = BUTTON_INFO;
    } else if (state->button1 > BUTTON_MIDDLE_LIMIT) {
        button = BUTTON_DOWN;
    } else if (state->button1 > BUTTON_LOWER_LIMIT) {
        button = BUTTON_VOLUME_DOWN;
    }

    if (button != previous_button &&
        previous_button != BUTTON_INFO_LONG_PRESS &&
        previous_button != BUTTON_DOWN_LONG_PRESS &&
        previous_button != BUTTON_VOLUME_DOWN_LONG_PRESS) {
        press_start_time = esp_timer_get_time_ms();
    }

    if (esp_timer_get_time_ms() > press_start_time + BUTTON_LONG_PRESS_MS) {
        if (button == BUTTON_INFO) {
            button = BUTTON_INFO_LONG_PRESS;
        } else if (button == BUTTON_DOWN) {
            button = BUTTON_DOWN_LONG_PRESS;
        } else if (button == BUTTON_VOLUME_DOWN) {
            button = BUTTON_VOLUME_DOWN_LONG_PRESS;
        }
    }

    previous_button = button;
    return button;
}

Button buttons_get_pressed(ButtonsState *state, MotionState *motion) {
    static Button previous_button = BUTTON_NONE;

    Button button = getPressedButton0(state);
    if (button == BUTTON_NONE) {
        button = getPressedButton1(state);
    }

    // Check compass as only our dev setup has no compass. We don't want to use this next piece in production!
    if (button == BUTTON_NONE && !motion->has_compass) {
        if (motion->accel_y > 0.3 && previous_button != BUTTON_VOLUME_DOWN) {
            button = BUTTON_VOLUME_UP;
        } else if (motion->accel_y < -0.3 && previous_button != BUTTON_VOLUME_UP) {
            button = BUTTON_VOLUME_DOWN;
        } else if (motion->gyro_x > 60 && previous_button != BUTTON_SOURCE) {
            button = BUTTON_UP;
        } else if (motion->gyro_x < -90 && previous_button != BUTTON_UP) {
            button = BUTTON_SOURCE;
        }
    }

    if (button == previous_button) {
        return BUTTON_NONE;
    }
    previous_button = button;
    return button;
}

void buttons_init() {
    adc_oneshot_init_channel(BUTTONS_ADC_CHANNEL_0);
    adc_oneshot_init_channel(BUTTONS_ADC_CHANNEL_1);
}