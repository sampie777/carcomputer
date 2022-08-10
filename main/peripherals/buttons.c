//
// Created by samuel on 28-7-22.
//

#include <driver/adc.h>
#include "buttons.h"
#include "../config.h"
#include "../utils.h"

int read_debounced(adc1_channel_t sensor_pin, uint8_t sample_count, int min_value, unsigned long debounce_cooldown_period, unsigned long min_press_time) {
    static int64_t last_action_time = 0;

    // Debounce button using cooldown period
    if (esp_timer_get_time_ms() < last_action_time + debounce_cooldown_period)
        return -1;

    // Debounce button using minimum pressed time
    int64_t pressStartTime = esp_timer_get_time_ms();
    double last_reading = average_read_channel(sensor_pin, sample_count);
    double max_reading = last_reading;
    while (last_reading > min_value && esp_timer_get_time_ms() < pressStartTime + min_press_time) {
        delay_ms(10);
        last_reading = average_read_channel(sensor_pin, sample_count);
        max_reading = max(max_reading, last_reading);
    }

    if (max_reading < min_value || esp_timer_get_time_ms() < pressStartTime + min_press_time)
        return 0;

    last_action_time = esp_timer_get_time_ms();
    return (int) max_reading;
}

Button getPressedButton0() {
    static Button previous_button = BUTTON_NONE;
    static int64_t press_start_time = 0;

    int sens = read_debounced(BUTTONS_ADC_CHANNEL_0, BUTTON_AVERAGE_READ_SAMPLES, BUTTON_LOWER_LIMIT, BUTTON_DEBOUNCE_COOLDOWN_PERIOD_MS, BUTTON_MIN_PRESS_TIME_MS);

    Button button = BUTTON_NONE;
    if (sens == -1) {
        button = previous_button;
    } else if (sens > BUTTON_UPPER_LIMIT) {
        button = BUTTON_SOURCE;
    } else if (sens > BUTTON_MIDDLE_LIMIT) {
        button = BUTTON_UP;
    } else if (sens > BUTTON_LOWER_LIMIT) {
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

Button getPressedButton1() {
    static Button previous_button = BUTTON_NONE;
    static int64_t press_start_time = 0;

    int sens = read_debounced(BUTTONS_ADC_CHANNEL_1, BUTTON_AVERAGE_READ_SAMPLES, BUTTON_LOWER_LIMIT, BUTTON_DEBOUNCE_COOLDOWN_PERIOD_MS, BUTTON_MIN_PRESS_TIME_MS);

    Button button = BUTTON_NONE;
    if (sens == -1) {
        button = previous_button;
    } else if (sens > BUTTON_UPPER_LIMIT) {
        button = BUTTON_INFO;
    } else if (sens > BUTTON_MIDDLE_LIMIT) {
        button = BUTTON_DOWN;
    } else if (sens > BUTTON_LOWER_LIMIT) {
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

Button buttons_get_pressed() {
    static Button previous_button = BUTTON_NONE;

    Button button = getPressedButton0();
    if (button == BUTTON_NONE) {
        button = getPressedButton1();
    }

    if (button == previous_button) {
        return BUTTON_NONE;
    }
    previous_button = button;
    return button;
}

void buttons_init() {
    adc1_config_channel_atten(BUTTONS_ADC_CHANNEL_0, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(BUTTONS_ADC_CHANNEL_1, ADC_ATTEN_DB_11);
}