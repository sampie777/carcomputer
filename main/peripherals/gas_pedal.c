//
// Created by samuel on 20-7-22.
//

#include <stdbool.h>
#include <driver/ledc.h>
#include <driver/adc.h>
#include <esp_timer.h>
#include "../config.h"
#include "gas_pedal.h"
#include "../return_codes.h"
#include "../utils.h"

int is_pedal_connected(double reading_0, double reading_1) {
    return reading_0 >= CAR_GAS_PEDAL_MIN_VALUE && reading_1 >= CAR_GAS_PEDAL_MIN_VALUE;
}

int gas_pedal_init_minimums(State *state) {
    printf("[GasPedal] Calibrating minimums\n");
    double reading_0 = average_read_channel(CAR_GAS_PEDAL_ADC_CHANNEL_0, CAR_GAS_PEDAL_ADC_SAMPLE_COUNT);
    double reading_1 = average_read_channel(CAR_GAS_PEDAL_ADC_CHANNEL_1, CAR_GAS_PEDAL_ADC_SAMPLE_COUNT);

    if (!is_pedal_connected(reading_0, reading_1)) {
        state->car.gas_pedal_connected = false;
        state->car.gas_pedal_0_min_value = 0;
        state->car.gas_pedal_1_min_value = 0;
        return RESULT_DISCONNECTED;
    }
    state->car.gas_pedal_connected = true;
    state->car.gas_pedal_0_min_value = (int) reading_0;
    state->car.gas_pedal_1_min_value = (int) reading_1;
    return RESULT_OK;
}

int gas_pedal_read(State *state) {
    double reading_0 = average_read_channel(CAR_GAS_PEDAL_ADC_CHANNEL_0, CAR_GAS_PEDAL_ADC_SAMPLE_COUNT);
    double reading_1 = average_read_channel(CAR_GAS_PEDAL_ADC_CHANNEL_1, CAR_GAS_PEDAL_ADC_SAMPLE_COUNT);

    if (!is_pedal_connected(reading_0, reading_1)) {
        state->car.gas_pedal_connected = false;
        return RESULT_DISCONNECTED;
    }
    state->car.gas_pedal_connected = true;

    if (state->car.gas_pedal_0_min_value == 0 && state->car.gas_pedal_1_min_value == 0) {
        gas_pedal_init_minimums(state);
    }

    if (state->car.gas_pedal_0_min_value > state->car.gas_pedal_1_min_value) {
        double difference = CAR_GAS_PEDAL_MAX_VALUE - state->car.gas_pedal_0_min_value;
        state->car.gas_pedal = (reading_0 - state->car.gas_pedal_0_min_value) / difference;
    } else {
        double difference = CAR_GAS_PEDAL_MAX_VALUE - state->car.gas_pedal_1_min_value;
        state->car.gas_pedal = (reading_1 - state->car.gas_pedal_1_min_value) / difference;
    }

    return RESULT_OK;
}

void gas_pedal_write(State *state) {
    if (state->car.gas_pedal_0_min_value == 0 && state->car.gas_pedal_1_min_value == 0) {
        gas_pedal_init_minimums(state);
    }

    uint32_t duty_cycle_0, duty_cycle_1;

    // Convert relative gas pedal value to absolute (10 bit, see config) duty cycle values for each sensor.
    if (state->car.gas_pedal_0_min_value > state->car.gas_pedal_1_min_value) {
        int difference = CAR_GAS_PEDAL_MAX_VALUE - state->car.gas_pedal_0_min_value;
        double factor = (double) state->car.gas_pedal_1_min_value / state->car.gas_pedal_0_min_value;
        duty_cycle_0 = (int) (state->car.gas_pedal_0_min_value + difference * state->cruise_control.virtual_gas_pedal);
        duty_cycle_1 = (int) (factor * duty_cycle_0);
    } else {
        int difference = CAR_GAS_PEDAL_MAX_VALUE - state->car.gas_pedal_1_min_value;
        double factor = (double) state->car.gas_pedal_0_min_value / state->car.gas_pedal_1_min_value;
        duty_cycle_1 = (int) (state->car.gas_pedal_1_min_value + difference * state->cruise_control.virtual_gas_pedal);
        duty_cycle_0 = (int) (factor * duty_cycle_1);
    }

    // Set PWM output
    ledc_set_duty(CAR_VIRTUAL_GAS_PEDAL_TIMER_SPEED_MODE, CAR_VIRTUAL_GAS_PEDAL_TIMER_CHANNEL_0, duty_cycle_0);
    ledc_set_duty(CAR_VIRTUAL_GAS_PEDAL_TIMER_SPEED_MODE, CAR_VIRTUAL_GAS_PEDAL_TIMER_CHANNEL_1, duty_cycle_1);

    ledc_update_duty(CAR_VIRTUAL_GAS_PEDAL_TIMER_SPEED_MODE, CAR_VIRTUAL_GAS_PEDAL_TIMER_CHANNEL_0);
    ledc_update_duty(CAR_VIRTUAL_GAS_PEDAL_TIMER_SPEED_MODE, CAR_VIRTUAL_GAS_PEDAL_TIMER_CHANNEL_1);
}

void gas_pedal_init(State *state) {
    // Init input
    adc1_config_channel_atten(CAR_GAS_PEDAL_ADC_CHANNEL_0, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(CAR_GAS_PEDAL_ADC_CHANNEL_1, ADC_ATTEN_DB_11);

    // Init output
    // 78 kHz gives max resolution of 10 bits: 80 MHz / 2^10 = 78 kHz
    uint32_t max_frequency = 80000000 / (0x01 << CAR_GAS_PEDAL_RESOLUTION);
    printf("[GasPedal] PWM frequency: %d Hz\n", max_frequency);
    ledc_timer_config_t config = {
            .speed_mode = CAR_VIRTUAL_GAS_PEDAL_TIMER_SPEED_MODE,
            .duty_resolution = CAR_GAS_PEDAL_RESOLUTION,
            .timer_num = CAR_VIRTUAL_GAS_PEDAL_TIMER,
            .freq_hz = max_frequency,
            .clk_cfg = LEDC_USE_APB_CLK,
    };

    ledc_timer_config(&config);

    ledc_channel_config_t channel_0_config = {
            .gpio_num = CAR_VIRTUAL_GAS_PEDAL_OUTPUT_PIN_0,
            .speed_mode = CAR_VIRTUAL_GAS_PEDAL_TIMER_SPEED_MODE,
            .channel = CAR_VIRTUAL_GAS_PEDAL_TIMER_CHANNEL_0,
            .intr_type = LEDC_INTR_DISABLE,
            .timer_sel = CAR_VIRTUAL_GAS_PEDAL_TIMER,
            .duty = 0,
            .hpoint = 0,
    };
    ledc_channel_config(&channel_0_config);

    ledc_channel_config_t channel_1_config = {
            .gpio_num = CAR_VIRTUAL_GAS_PEDAL_OUTPUT_PIN_1,
            .speed_mode = CAR_VIRTUAL_GAS_PEDAL_TIMER_SPEED_MODE,
            .channel = CAR_VIRTUAL_GAS_PEDAL_TIMER_CHANNEL_1,
            .intr_type = LEDC_INTR_DISABLE,
            .timer_sel = CAR_VIRTUAL_GAS_PEDAL_TIMER,
            .duty = 0,
            .hpoint = 0,
    };
    ledc_channel_config(&channel_1_config);

    gas_pedal_init_minimums(state);
}