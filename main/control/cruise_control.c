//
// Created by samuel on 27-7-22.
//

#include "cruise_control.h"

#include <tgmath.h>

#include "../config.h"
#include "../utils.h"
#include "../peripherals/gas_pedal.h"


void cruise_control_apply_pid(State* state) {
    static double previous_error = 0;
    static double previous_integral = 0;
    static int64_t last_iteration_time = 0;
    static bool is_pedal_released_after_init = false;

    if (!state->cruise_control.enabled) {
        previous_error = 0;
        previous_integral = 0;
        last_iteration_time = 0;
        state->cruise_control.virtual_gas_pedal = 0;
        is_pedal_released_after_init = false;
        return;
    }

    //    if (!_isSpeedControl) {
    //        return;
    //    }

    if (esp_timer_get_time_ms() < last_iteration_time + CRUISE_CONTROL_PID_ITERATION_TIME) return;
    int64_t iterationTime = esp_timer_get_time_ms() - last_iteration_time;
    last_iteration_time = esp_timer_get_time_ms();

    // Calculate PID
    double error = state->cruise_control.target_speed - state->car.speed;
    double integral = previous_integral + error * (double)iterationTime;
    double derivative = (error - previous_error) / (double)iterationTime;
    double output = state->cruise_control.initial_control_value
        + state->cruise_control.pidKp * error
        + state->cruise_control.pidKi * integral
        + state->cruise_control.pidKd * derivative;

    // Anti reset wind-up
    if (output >= 1.0) {
        output = 1.0;
        integral = previous_integral;
    }
    else if (output <= 0.0) {
        output = 0.0;
        integral = previous_integral;
    }

    previous_error = error;
    previous_integral = integral;

    if (state->car.gas_pedal > 0.1) {
        if (!is_pedal_released_after_init) return;

        // Pedal override interaction
        double override_control_value = max(0.0, min(1.0, state->cruise_control.control_value + state->car.gas_pedal));
        state->cruise_control.virtual_gas_pedal = override_control_value;
        return;
    }
    is_pedal_released_after_init = true;

    // Apply PID
    state->cruise_control.control_value = output;
    state->cruise_control.virtual_gas_pedal = state->cruise_control.control_value;
    // printf("  cc: %lf %%; %lf km/h of %lf km/h\n",
    //        state->cruise_control.virtual_gas_pedal,
    //        state->car.speed,
    //        state->cruise_control.target_speed);
}

void cruise_control_step(State* state) {
    static uint8_t car_was_connected = false;
    static uint8_t cruise_control_was_enabled = false;
    static int64_t gas_pedal_enable_time = 0;

    // Safety checks
    if (!state->car.gas_pedal_connected) {
        if (state->cruise_control.enabled) printf("Disconnecting cruise control because of disconnected gas pedal\n");
        state->cruise_control.enabled = false;
    }

    if (state->car.is_connected) {
        if (state->car.is_braking || state->car.rpm > CRUISE_CONTROL_MAX_RPM_LIMIT) {
            if (state->cruise_control.enabled) printf("Disconnecting cruise control because of braking or high refs\n");
            state->cruise_control.enabled = false;
        }
    }
    else if (car_was_connected) {
        if (state->cruise_control.enabled) printf("Disconnecting cruise control because of disconnected car\n");
        state->cruise_control.enabled = false;
    }
    car_was_connected = state->car.is_connected;

    // Check if cruise control was just now enabled
    if (state->cruise_control.enabled && state->cruise_control.enabled != cruise_control_was_enabled) {
        state->cruise_control.target_speed = round(state->car.speed);
        state->cruise_control.initial_control_value = state->car.gas_pedal;
        state->cruise_control.virtual_gas_pedal = min(1.0, max(0.0, state->car.gas_pedal));
        gas_pedal_enable_time = esp_timer_get_time_ms() + CAR_VIRTUAL_GAS_PEDAL_RISE_TIME_MS;

        printf("Cruise control enabled. \n"
               "\tpidKp = %lf; pidKi = %lf; pidKd = %lf\n"
               "\ttarget_speed = %lf\n"
               "\tinitial_control_value = %lf\n"
               "\tvirtual_gas_pedal = %lf\n"
               "\t",
               state->cruise_control.pidKp,
               state->cruise_control.pidKi,
               state->cruise_control.pidKd,
               state->cruise_control.target_speed,
               state->cruise_control.initial_control_value,
               state->cruise_control.virtual_gas_pedal
        );
    }
    cruise_control_was_enabled = state->cruise_control.enabled;

    // Disable or enable gas pedal after pedal output rise time
    if (!state->cruise_control.enabled) {
        gas_pedal_enable(false);
    }
    else if (gas_pedal_enable_time == 0 || esp_timer_get_time_ms() > gas_pedal_enable_time) {
        gas_pedal_enable(true);
        // Reset time to 0 to prevent bugs when get_time_ms overflows
        gas_pedal_enable_time = 0;
    }

    cruise_control_apply_pid(state);

    gas_pedal_write(state);
}
