//
// Created by samuel on 20-7-22.
//

#include <stdbool.h>
#include <driver/ledc.h>
#include <driver/adc.h>
#include <esp_timer.h>
#include "../config.h"
#include "gas_pedal.h"

#include <tgmath.h>

#include "../return_codes.h"
#include "../utils.h"

void gas_pedal_enable(uint8_t enable) {
    gpio_set_level(CAR_VIRTUAL_GAS_PEDAL_ENABLE_PIN, enable ? 1 : 0);
}

int is_pedal_connected(double reading_0, double reading_1) {
    double min_voltage = min(CAR_GAS_PEDAL0_MIN_VOLTS, CAR_GAS_PEDAL1_MIN_VOLTS) / 2;
    return reading_0 >= min_voltage && reading_1 >= min_voltage;
}

double read_pedal_volts(adc1_channel_t channel, int sample_count_factor) {
    double reading = average_read_channel(channel, sample_count_factor * CAR_GAS_PEDAL_ADC_SAMPLE_COUNT);
    if (reading > 17) {
        return 0.0015795 * reading + 0.2702;
    }

    return -0.0004 * pow(reading, 2) + 0.0234 * reading;
}

void read_pedals(State *state, int sample_count_factor) {
    state->car.gas_pedal_0_volts = read_pedal_volts(CAR_GAS_PEDAL_ADC_CHANNEL_0, sample_count_factor);

    // Give ADC time to settle for 10 clock cycles, otherwise next reading will be influenced
    for (volatile int i = 0; i < 10; i++) {
    }

    state->car.gas_pedal_1_volts = read_pedal_volts(CAR_GAS_PEDAL_ADC_CHANNEL_1, sample_count_factor);
}

void set_pedal_volts(ledc_channel_t channel, double voltage) {
    uint16_t max_cycle = pow(2, CAR_GAS_PEDAL_RESOLUTION) - 1;
    uint16_t duty_cycle = (uint16_t) (max(0, min(voltage, 5)) / 5 * max_cycle);
    ledc_set_duty(CAR_VIRTUAL_GAS_PEDAL_TIMER_SPEED_MODE, channel, duty_cycle);
    ledc_update_duty(CAR_VIRTUAL_GAS_PEDAL_TIMER_SPEED_MODE, channel);
}

int gas_pedal_init_minimums(State *state) {
    read_pedals(state, 5);

    if (!is_pedal_connected(state->car.gas_pedal_0_volts, state->car.gas_pedal_1_volts)) {
        state->car.gas_pedal_connected = false;
        state->car.gas_pedal_0_min_value_volts = 0;
        state->car.gas_pedal_1_min_value_volts = 0;
        state->car.gas_pedal_0_max_value_volts = 0;
        state->car.gas_pedal_1_max_value_volts = 0;
        return RESULT_DISCONNECTED;
    }

    state->car.gas_pedal_connected = true;
    if (state->car.gas_pedal_0_volts > state->car.gas_pedal_1_volts) {
        state->car.gas_pedal_0_min_value_volts = max(CAR_GAS_PEDAL0_MIN_VOLTS, CAR_GAS_PEDAL1_MIN_VOLTS);
        state->car.gas_pedal_1_min_value_volts = min(CAR_GAS_PEDAL0_MIN_VOLTS, CAR_GAS_PEDAL1_MIN_VOLTS);
        state->car.gas_pedal_0_max_value_volts = max(CAR_GAS_PEDAL0_MAX_VOLTS, CAR_GAS_PEDAL1_MAX_VOLTS);
        state->car.gas_pedal_1_max_value_volts = min(CAR_GAS_PEDAL0_MAX_VOLTS, CAR_GAS_PEDAL1_MAX_VOLTS);
    } else {
        state->car.gas_pedal_0_min_value_volts = min(CAR_GAS_PEDAL0_MIN_VOLTS, CAR_GAS_PEDAL1_MIN_VOLTS);
        state->car.gas_pedal_1_min_value_volts = max(CAR_GAS_PEDAL0_MIN_VOLTS, CAR_GAS_PEDAL1_MIN_VOLTS);
        state->car.gas_pedal_0_max_value_volts = min(CAR_GAS_PEDAL0_MAX_VOLTS, CAR_GAS_PEDAL1_MAX_VOLTS);
        state->car.gas_pedal_1_max_value_volts = max(CAR_GAS_PEDAL0_MAX_VOLTS, CAR_GAS_PEDAL1_MAX_VOLTS);
    }
    return RESULT_OK;
}

int gas_pedal_read(State *state) {
    read_pedals(state, 1);

    if (!is_pedal_connected(state->car.gas_pedal_0_volts, state->car.gas_pedal_1_volts)) {
        state->car.gas_pedal_connected = false;
        return RESULT_DISCONNECTED;
    }
    state->car.gas_pedal_connected = true;

    if (state->car.gas_pedal_0_min_value_volts == 0 && state->car.gas_pedal_1_min_value_volts == 0) {
        gas_pedal_init_minimums(state);
    }

    if (state->car.gas_pedal_0_min_value_volts > state->car.gas_pedal_1_min_value_volts) {
        double difference = state->car.gas_pedal_0_max_value_volts - state->car.gas_pedal_0_min_value_volts;
        state->car.gas_pedal = max(
            0.0, (state->car.gas_pedal_0_volts - state->car.gas_pedal_0_min_value_volts) / difference);
    } else {
        double difference = state->car.gas_pedal_1_max_value_volts - state->car.gas_pedal_1_min_value_volts;
        state->car.gas_pedal = max(
            0.0, (state->car.gas_pedal_1_volts - state->car.gas_pedal_1_min_value_volts) / difference);
    }

    return RESULT_OK;
}

void gas_pedal_write(State *state) {
    if (state->car.gas_pedal_0_min_value_volts == 0 && state->car.gas_pedal_1_min_value_volts == 0) {
        gas_pedal_init_minimums(state);
    }

    double target_voltage0 = scale(state->cruise_control.virtual_gas_pedal, state->car.gas_pedal_0_min_value_volts,
                                   state->car.gas_pedal_0_max_value_volts);
    double target_voltage1 = scale(state->cruise_control.virtual_gas_pedal, state->car.gas_pedal_1_min_value_volts,
                                   state->car.gas_pedal_1_max_value_volts);

    // Set PWM output
    set_pedal_volts(CAR_VIRTUAL_GAS_PEDAL_TIMER_CHANNEL_0, target_voltage0);
    set_pedal_volts(CAR_VIRTUAL_GAS_PEDAL_TIMER_CHANNEL_1, target_voltage1);
}

/**
 *
 * @param state App state.
 * @param frequency Frequency for PWM to use. Set to 0 to use max available frequency.
 */
void gas_pedal_init(State *state, uint32_t frequency) {
    // Init input
    adc1_config_channel_atten(CAR_GAS_PEDAL_ADC_CHANNEL_0, ADC_ATTEN_DB_12);
    adc1_config_channel_atten(CAR_GAS_PEDAL_ADC_CHANNEL_1, ADC_ATTEN_DB_12);

    // Init output
    gpio_set_direction(CAR_VIRTUAL_GAS_PEDAL_ENABLE_PIN, GPIO_MODE_OUTPUT);
    gas_pedal_enable(false);

    // 78 kHz gives max resolution of 10 bits: 80 MHz / 2^10 = 78 kHz
    if (frequency == 0) frequency = 80000000 / (0x01 << CAR_GAS_PEDAL_RESOLUTION);

    printf("[GasPedal] PWM frequency: %lu Hz\n", frequency);
    ledc_timer_config_t config = {
        .speed_mode = CAR_VIRTUAL_GAS_PEDAL_TIMER_SPEED_MODE,
        .duty_resolution = CAR_GAS_PEDAL_RESOLUTION,
        .timer_num = CAR_VIRTUAL_GAS_PEDAL_TIMER,
        .freq_hz = frequency,
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
