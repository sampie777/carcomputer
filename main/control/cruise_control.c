//
// Created by samuel on 27-7-22.
//

#include "cruise_control.h"
#include <stdio.h>
#include <math.h>
#include "../utils.h"
#include "../peripherals/gas_pedal.h"

void cruise_control_update_graph(State *state) {
    for (int i = 0; i < CRUISE_CONTROL_GRAPH_SIZE - 1; i++) {
        state->speed_control.cruise_control.graph.virtual_gas_pedal[i] = state->speed_control.cruise_control.graph.virtual_gas_pedal[i + 1];
    }
    state->speed_control.cruise_control.graph.virtual_gas_pedal[CRUISE_CONTROL_GRAPH_SIZE - 1] =
            state->speed_control.cruise_control.enabled
                ? (int) (state->speed_control.virtual_gas_pedal * 100.0)
                : -1;
}

void cruise_control_apply_pid(State *state) {
    static double previous_error = 0;
    static double previous_integral = 0;
    static int64_t last_iteration_time = 0;
    static bool is_pedal_released_after_init = false;

    if (esp_timer_get_time_ms() < last_iteration_time + CRUISE_CONTROL_PID_ITERATION_TIME) return;
    int64_t iteration_time = last_iteration_time == 0
                                 ? CRUISE_CONTROL_PID_ITERATION_TIME
                                 : esp_timer_get_time_ms() - last_iteration_time;
    last_iteration_time = esp_timer_get_time_ms();

    if (!state->speed_control.cruise_control.enabled) {
        previous_error = 0;
        previous_integral = 0;
        is_pedal_released_after_init = false;

        cruise_control_update_graph(state);
        return;
    }

    // If pedal is still depressed when cruise control is engaged, just keep using the current pedal value,
    // until the user releases the pedal. Only after that we can switch over to the actual cruise control logic.
    if (!is_pedal_released_after_init && state->car.gas_pedal > CRUISE_CONTROL_OVERRIDE_PEDAL_MIN) {
        state->speed_control.virtual_gas_pedal = state->car.gas_pedal;
        return;
    }
    is_pedal_released_after_init = true;

    // Calculate PID
    double error = state->speed_control.cruise_control.target_speed - state->car.speed;

    state->speed_control.cruise_control.error = error;
    state->speed_control.cruise_control.integral = previous_integral + error * (double) iteration_time;
    state->speed_control.cruise_control.derivative = (error - previous_error) / (double) iteration_time;

    // Prevent over shooting after overshooting 1.5 km/h too fast
    if (error < -1 * CRUISE_CONTROL_MAX_SPEED_OVERSHOOT) {
        state->speed_control.virtual_gas_pedal *= 0.85;
        if (state->speed_control.cruise_control.integral > 0) {
            state->speed_control.cruise_control.integral *= 0.75;
        }
        if (error < -2 * CRUISE_CONTROL_MAX_SPEED_OVERSHOOT) {
            state->speed_control.virtual_gas_pedal *= 0.7;
            state->speed_control.cruise_control.integral *= 0.8;
        }
    }

    // Prevent car from accelerating too fast
    if (state->car.acceleration > CRUISE_CONTROL_MAX_ACCELERATION_MS2_LOWER_BOUND) {
        // state->speed_control.cruise_control.integral *= 0.95;
    }

    double output = state->speed_control.cruise_control.initial_control_value
                    + state->speed_control.cruise_control.pidKp * error
                    + state->speed_control.cruise_control.pidKi * state->speed_control.cruise_control.integral
                    + state->speed_control.cruise_control.pidKd * state->speed_control.cruise_control.derivative;

    // Anti reset wind-up
    if (output >= 1.0) {
        output = 1.0;
        state->speed_control.cruise_control.integral = previous_integral;
    } else if (output <= 0.0) {
        output = 0.0;
        state->speed_control.cruise_control.integral = previous_integral;
    }

    previous_error = error;
    previous_integral = state->speed_control.cruise_control.integral;

    if (state->car.gas_pedal > CRUISE_CONTROL_OVERRIDE_PEDAL_MIN) {
        // Pedal override interaction
        double override_control_value = max(0.0, min(1.0, state->speed_control.cruise_control.control_value + state->car.gas_pedal));
        state->speed_control.virtual_gas_pedal = override_control_value;
    } else {
        // Apply PID
        state->speed_control.cruise_control.control_value = output;
        state->speed_control.virtual_gas_pedal = state->speed_control.cruise_control.control_value * 0.12
                                                 + state->speed_control.virtual_gas_pedal * 0.88;
    }

    cruise_control_update_graph(state);
}

bool cruise_control_safety_checks(State *state, uint8_t car_was_connected) {
    static int64_t gear_in_neutral_since_time = -1;

    // Safety checks
    if (!state->car.gas_pedal_connected) {
        if (state->speed_control.cruise_control.enabled) printf("Disconnecting cruise control because of disconnected gas pedal\n");
        return false;
    }

    if (!state->car.is_connected) {
        if (car_was_connected) {
            if (state->speed_control.cruise_control.enabled) printf("Disconnecting cruise control because of disconnected car\n");
            return false;
        }
        return true;
    }

    if (state->car.is_braking) {
        if (state->speed_control.cruise_control.enabled) printf("Disconnecting cruise control because of braking\n");
        return false;
    }

    if (state->car.rpm > CRUISE_CONTROL_MAX_RPM_LIMIT) {
        if (state->speed_control.cruise_control.enabled) printf("Disconnecting cruise control because of high refs\n");
        return false;
    }

    if (state->car.speed > 1 && state->car.estimated_gear == GearNeutral) {
        // Disconnect CC if the car is out of gear for longer than 600 ms while driving
        if (gear_in_neutral_since_time < 0) {
            gear_in_neutral_since_time = esp_timer_get_time_ms();
        } else if (esp_timer_get_time_ms() > gear_in_neutral_since_time + 600) {
            if (state->speed_control.cruise_control.enabled) printf("Disconnecting cruise control because of clutch depressed\n");
            return false;
        }
    } else {
        gear_in_neutral_since_time = -1;
    }

    if (state->car.speed > 1 && state->car.is_parking_brake_on) {
        if (state->speed_control.cruise_control.enabled) printf("Disconnecting cruise control because of parking brake\n");
        return false;
    }

    return true;
}

void cruise_control_step(State *state) {
    static uint8_t cruise_control_was_enabled = false;
    static uint8_t car_was_connected = false;
    static int64_t gas_pedal_enable_time = 0;
    static double previous_target_speed = 0; // Uses just to see if the current target speed has been updated

    if (state->speed_control.cruise_control.target_speed != previous_target_speed) {
        printf("target speed: %lf\n", state->speed_control.cruise_control.target_speed);
        previous_target_speed = state->speed_control.cruise_control.target_speed;
    }

    if (!cruise_control_safety_checks(state, car_was_connected)) {
        state->speed_control.cruise_control.enabled = false;
    }
    car_was_connected = state->car.is_connected;

    if (state->speed_control.cruise_control.enabled != cruise_control_was_enabled) {
        // Check if cruise control was just now enabled
        if (state->speed_control.cruise_control.enabled) {
            state->speed_control.cruise_control.target_speed = round(state->car.speed);
            state->speed_control.cruise_control.initial_control_value = state->car.gas_pedal;
            state->speed_control.virtual_gas_pedal = 0;
            gas_pedal_enable_time = esp_timer_get_time_ms() + CAR_VIRTUAL_GAS_PEDAL_RISE_TIME_MS;

            printf("Cruise control enabled. \n"
                   "\tpidKp = %lf; pidKi = %lf; pidKd = %lf\n"
                   "\ttarget_speed = %lf\n"
                   "\tinitial_control_value = %lf\n"
                   "\tvirtual_gas_pedal = %lf\n",
                   state->speed_control.cruise_control.pidKp,
                   state->speed_control.cruise_control.pidKi,
                   state->speed_control.cruise_control.pidKd,
                   state->speed_control.cruise_control.target_speed,
                   state->speed_control.cruise_control.initial_control_value,
                   state->speed_control.virtual_gas_pedal
            );
        } else {
            state->speed_control.cruise_control.previous_target_speed = state->speed_control.cruise_control.target_speed;
        }
    }
    cruise_control_was_enabled = state->speed_control.cruise_control.enabled;

    // Disable or enable gas pedal after pedal output rise time
    if (!state->speed_control.cruise_control.enabled) {
        // Disabling is done in control.c due to multiple speed controls wanting to control the pedal
    } else if (gas_pedal_enable_time == 0 || esp_timer_get_time_ms() > gas_pedal_enable_time) {
        gas_pedal_enable(true);
        // Reset time to 0 to prevent bugs when get_time_ms overflows
        gas_pedal_enable_time = 0;
    }

    cruise_control_apply_pid(state);

    gas_pedal_write(state);
}

void cruise_control_config_apply_factor(State *state, int8_t sign) {
    double value = pow(10, state->display.pid_config.factor) * sign;
    switch (state->display.pid_config.selected_type) {
        case PidConfigScreenOptionsType_P:
            state->speed_control.cruise_control.pidKp += value;
            break;
        case PidConfigScreenOptionsType_I:
            state->speed_control.cruise_control.pidKi += value;
            break;
        case PidConfigScreenOptionsType_D:
            state->speed_control.cruise_control.pidKd += value;
            break;
        default:
            break;
    }
}
