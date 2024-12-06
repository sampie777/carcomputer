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
    int64_t iteration_time = last_iteration_time == 0
                                 ? CRUISE_CONTROL_PID_ITERATION_TIME
                                 : esp_timer_get_time_ms() - last_iteration_time;
    last_iteration_time = esp_timer_get_time_ms();

    // If pedal is still depressed when cruise control is engaged, just keep using the current pedal value,
    // until the user releases the pedal. Only after that we can switch over to the actual cruise control logic.
    if (!is_pedal_released_after_init && state->car.gas_pedal > CRUISE_CONTROL_OVERRIDE_PEDAL_MIN) {
        state->cruise_control.virtual_gas_pedal = state->car.gas_pedal;
        return;
    }
    is_pedal_released_after_init = true;

    // Calculate PID
    double error = state->cruise_control.target_speed - state->car.speed;
    double integral = previous_integral + error * (double) iteration_time;
    double derivative = (error - previous_error) / (double) iteration_time;
    double output = state->cruise_control.initial_control_value
        + state->cruise_control.pidKp * error
        + state->cruise_control.pidKi * integral
        + state->cruise_control.pidKd * derivative;

    // Anti reset wind-up
    if (output >= 1.0) {
        output = 1.0;
        integral = previous_integral;
    } else if (output <= 0.0) {
        output = 0.0;
        integral = previous_integral;
    }

    previous_error = error;
    previous_integral = integral;

    if (state->car.gas_pedal > CRUISE_CONTROL_OVERRIDE_PEDAL_MIN) {
        // Pedal override interaction
        double override_control_value = max(0.0, min(1.0, state->cruise_control.control_value + state->car.gas_pedal));
        state->cruise_control.virtual_gas_pedal = override_control_value;
        return;
    }

    // Apply PID
    state->cruise_control.control_value = output;
    state->cruise_control.virtual_gas_pedal = state->cruise_control.control_value;
    // printf("  cc: %lf %%; %lf km/h of %lf km/h; %f; %lld ms\n",
    //        state->cruise_control.control_value,
    //        state->car.speed,
    //        state->cruise_control.target_speed,
    //        error,
    //        iteration_time);
}

void cruise_control_safety_checks(State* state, uint8_t car_was_connected) {
    static int64_t gear_in_neutral_since_time = -1;

    // Safety checks
    if (!state->car.gas_pedal_connected) {
        if (state->cruise_control.enabled) printf("Disconnecting cruise control because of disconnected gas pedal\n");
        state->cruise_control.enabled = false;
    }

    if (!state->car.is_connected) {
        if (car_was_connected) {
            if (state->cruise_control.enabled) printf("Disconnecting cruise control because of disconnected car\n");
            state->cruise_control.enabled = false;
        }
        return;
    }

    if (state->car.is_braking) {
        if (state->cruise_control.enabled) printf("Disconnecting cruise control because of braking\n");
        state->cruise_control.enabled = false;
    }

    if (state->car.rpm > CRUISE_CONTROL_MAX_RPM_LIMIT) {
        if (state->cruise_control.enabled) printf("Disconnecting cruise control because of high refs\n");
        state->cruise_control.enabled = false;
    }

    if (state->car.speed > 0 && state->car.estimated_gear == GearNeutral) {
        // Disconnect CC if the car is out of gear for longer than 600 ms while driving
        if (gear_in_neutral_since_time < 0) {
            gear_in_neutral_since_time = esp_timer_get_time_ms();
        } else if (esp_timer_get_time_ms() > gear_in_neutral_since_time + 600) {
            if (state->cruise_control.enabled) printf("Disconnecting cruise control because of clutch depressed\n");
            state->cruise_control.enabled = false;
        }
    } else {
        gear_in_neutral_since_time = -1;
    }

    if (state->car.speed > 0 && state->car.is_parking_brake_on) {
        if (state->cruise_control.enabled) printf("Disconnecting cruise control because of parking brake\n");
        state->cruise_control.enabled = false;
    }
}

void cruise_control_step(State* state) {
    static uint8_t cruise_control_was_enabled = false;
    static uint8_t car_was_connected = false;
    static int64_t gas_pedal_enable_time = 0;
    static double previous_target_speed = 0; // Uses just to see if the current target speed has been updated

    if (state->cruise_control.target_speed != previous_target_speed) {
        printf("target speed: %lf\n", state->cruise_control.target_speed);
        previous_target_speed = state->cruise_control.target_speed;
    }

    cruise_control_safety_checks(state, car_was_connected);
    car_was_connected = state->car.is_connected;

    if (state->cruise_control.enabled != cruise_control_was_enabled) {
        // Check if cruise control was just now enabled
        if (state->cruise_control.enabled) {
            state->cruise_control.target_speed = round(state->car.speed);
            state->cruise_control.initial_control_value = state->car.gas_pedal;
            state->cruise_control.virtual_gas_pedal = 0;
            gas_pedal_enable_time = esp_timer_get_time_ms() + CAR_VIRTUAL_GAS_PEDAL_RISE_TIME_MS;

            printf("Cruise control enabled. \n"
                   "\tpidKp = %lf; pidKi = %lf; pidKd = %lf\n"
                   "\ttarget_speed = %lf\n"
                   "\tinitial_control_value = %lf\n"
                   "\tvirtual_gas_pedal = %lf\n",
                   state->cruise_control.pidKp,
                   state->cruise_control.pidKi,
                   state->cruise_control.pidKd,
                   state->cruise_control.target_speed,
                   state->cruise_control.initial_control_value,
                   state->cruise_control.virtual_gas_pedal
            );
        } else {
            state->cruise_control.previous_target_speed = state->cruise_control.target_speed;
        }
    }
    cruise_control_was_enabled = state->cruise_control.enabled;

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
