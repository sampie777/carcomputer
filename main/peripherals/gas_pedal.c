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
    return reading_0 >= CAR_GAS_PEDAL_MIN_VOLTS && reading_1 >= CAR_GAS_PEDAL_MIN_VOLTS;
}

double read_pedal_volts(adc1_channel_t channel, int sample_count_factor) {
    double reading = average_read_channel(channel, sample_count_factor * CAR_GAS_PEDAL_ADC_SAMPLE_COUNT);
    return reading / 2760 * 4.8;
}

void read_pedals(State *state, int sample_count_factor) {
    state->car.gas_pedal_0_volts = read_pedal_volts(CAR_GAS_PEDAL_ADC_CHANNEL_0, sample_count_factor);

    // Give ADC time to settle for 10 clock cycles, otherwise next reading will be influenced
    for(volatile int i = 0; i < 10; i++) {}

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
        return RESULT_DISCONNECTED;
    }

    state->car.gas_pedal_connected = true;
    state->car.gas_pedal_0_min_value_volts = state->car.gas_pedal_0_volts;
    state->car.gas_pedal_1_min_value_volts = state->car.gas_pedal_1_volts;
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
        double difference = CAR_GAS_PEDAL_MAX_VOLTS - state->car.gas_pedal_0_min_value_volts;
        state->car.gas_pedal = (state->car.gas_pedal_0_volts - state->car.gas_pedal_0_min_value_volts) / difference;
    } else {
        double difference = CAR_GAS_PEDAL_MAX_VOLTS - state->car.gas_pedal_1_min_value_volts;
        state->car.gas_pedal = (state->car.gas_pedal_1_volts - state->car.gas_pedal_1_min_value_volts) / difference;
    }

    return RESULT_OK;
}

void gas_pedal_write(State *state) {
    if (state->car.gas_pedal_0_min_value_volts == 0 && state->car.gas_pedal_1_min_value_volts == 0) {
        gas_pedal_init_minimums(state);
    }

    double base_voltage_min = min(state->car.gas_pedal_0_min_value_volts, state->car.gas_pedal_1_min_value_volts);
    double base_voltage_max = max(state->car.gas_pedal_0_min_value_volts, state->car.gas_pedal_1_min_value_volts);

    double difference = CAR_GAS_PEDAL_MAX_VOLTS - base_voltage_max;
    double target_voltage = base_voltage_max + difference * state->cruise_control.virtual_gas_pedal;

    double factor;
    if (state->car.gas_pedal_0_min_value_volts == state->car.gas_pedal_1_min_value_volts) factor = 1;
    else if (base_voltage_max == 0) factor = 0;
    else factor = base_voltage_min / base_voltage_max;

    double voltage0, voltage1;
    if (state->car.gas_pedal_0_min_value_volts > state->car.gas_pedal_1_min_value_volts) {
        voltage0 = target_voltage;
        voltage1 = factor * target_voltage;
    } else {
        voltage0 = factor * target_voltage;
        voltage1 = target_voltage;
    }

    // Set PWM output
    set_pedal_volts(CAR_VIRTUAL_GAS_PEDAL_TIMER_CHANNEL_0, voltage0);
    set_pedal_volts(CAR_VIRTUAL_GAS_PEDAL_TIMER_CHANNEL_1, voltage1);
}

void gas_pedal_init(State *state) {
    // Init input
    adc1_config_channel_atten(CAR_GAS_PEDAL_ADC_CHANNEL_0, ADC_ATTEN_DB_12);
    adc1_config_channel_atten(CAR_GAS_PEDAL_ADC_CHANNEL_1, ADC_ATTEN_DB_12);

    // Init output
    gpio_set_direction(CAR_VIRTUAL_GAS_PEDAL_ENABLE_PIN, GPIO_MODE_OUTPUT);
    gas_pedal_enable(false);

    // 78 kHz gives max resolution of 10 bits: 80 MHz / 2^10 = 78 kHz
    uint32_t max_frequency = 80000000 / (0x01 << CAR_GAS_PEDAL_RESOLUTION);
    printf("[GasPedal] PWM frequency: %lu Hz\n", max_frequency);
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
