//
// Created by samuel on 27-7-22.
//

#include "cruise_control.h"
#include "../utils.h"
#include "../peripherals/gas_pedal.h"


void cruise_control_apply_pid(State *state) {
    static double previous_error = 0;
    static double previous_integral = 0;
    static int64_t last_iteration_time = 0;

    if (!state->cruise_control.enabled) {
        previous_error = 0;
        previous_integral = 0;
        last_iteration_time = 0;
        return;
    }

//    if (!_isSpeedControl) {
//        return;
//    }

    if (esp_timer_get_time_ms() < last_iteration_time + CRUISE_CONTROL_PID_ITERATION_TIME) {
        return;
    }
    int64_t iterationTime = esp_timer_get_time_ms() - last_iteration_time;
    last_iteration_time = esp_timer_get_time_ms();

    // Calculate PID
    double error = state->cruise_control.target_speed - state->car.speed;
    double integral = previous_integral + error * (double) iterationTime;
    double derivative = (error - previous_error) / (double) iterationTime;
    double output = state->cruise_control.initial_control_value
                    + CRUISE_CONTROL_PID_Kp * error
                    + CRUISE_CONTROL_PID_Ki * integral
                    + CRUISE_CONTROL_PID_Kd * derivative;

    // Anti reset wind-up
    if (output >= 1.0) {
        output = 1.0;
        integral = previous_integral;
    } else if (output <= 0.0) {
        output = 0.0;
        integral = previous_integral;
    }
    output = max(0.0, min(1.0, output));

    previous_error = error;
    previous_integral = integral;

    if (state->car.gas_pedal > 0.01) {
        // Pedal override interaction
        double overrideControlValue = max(0.0, min(1.0, state->cruise_control.control_value + state->car.gas_pedal));
        state->cruise_control.virtual_gas_pedal = overrideControlValue;
    } else {
        // Apply PID
        state->cruise_control.control_value = output;
        state->cruise_control.virtual_gas_pedal = state->cruise_control.control_value;
    }
}

void cruise_control_step(State *state) {
    static uint8_t car_was_connected = false;
    static uint8_t cruise_control_was_enabled = false;
    static int64_t gas_pedal_enable_time = 0;

    // Safety checks
    if (!state->car.gas_pedal_connected) {
        state->cruise_control.enabled = false;
    }

    if (state->car.is_connected) {
        if (state->car.is_braking || state->car.rpm > CRUISE_CONTROL_MAX_RPM_LIMIT) {
            state->cruise_control.enabled = false;
        }
    } else if (car_was_connected) {
        state->cruise_control.enabled = false;
    }
    car_was_connected = state->car.is_connected;

    // Check if cruise control was just now enabled
    if (state->cruise_control.enabled && state->cruise_control.enabled != cruise_control_was_enabled) {
        cruise_control_was_enabled = state->cruise_control.enabled;
        state->cruise_control.target_speed = state->car.speed;
        state->cruise_control.initial_control_value = state->car.gas_pedal;
        gas_pedal_enable_time = esp_timer_get_time_ms() + CAR_VIRTUAL_GAS_PEDAL_RISE_TIME_MS;
    }

    // Disable or enable gas pedal after pedal output rise time
    if (!state->cruise_control.enabled) {
        gas_pedal_enable(false);
    } else if (gas_pedal_enable_time == 0 || esp_timer_get_time_ms() > gas_pedal_enable_time) {
        gas_pedal_enable(true);
        // Reset time to 0 to prevent bugs when get_time_ms overflows
        gas_pedal_enable_time = 0;
    }

    cruise_control_apply_pid(state);

    gas_pedal_write(state);
}