//
// Created by Samuel-Anton Jansen on 2025/11/19.
//

#include <stdio.h>
#include "speed_control.h"
#include "cruise_control.h"
#include "../utils.h"
#include "../peripherals/gas_pedal.h"

void pedal_control_apply_pid(State *state) {
    static bool is_pedal_released_after_init = false;

    if (!state->speed_control.pedal_control.enabled) {
        is_pedal_released_after_init = false;
        return;
    }

    // If pedal is still depressed when cruise control is engaged, just keep using the current pedal value,
    // until the user releases the pedal. Only after that we can switch over to the actual cruise control logic.
    if (!is_pedal_released_after_init && state->car.gas_pedal > CRUISE_CONTROL_OVERRIDE_PEDAL_MIN) {
        state->speed_control.virtual_gas_pedal = state->car.gas_pedal;
        return;
    }
    is_pedal_released_after_init = true;

    if (state->car.gas_pedal > CRUISE_CONTROL_OVERRIDE_PEDAL_MIN) {
        // Pedal override interaction
        double override_control_value = max(0.0, min(1.0, state->speed_control.pedal_control.target_value + state->car.gas_pedal));
        state->speed_control.virtual_gas_pedal = override_control_value;
    } else {
        state->speed_control.virtual_gas_pedal = state->speed_control.pedal_control.target_value * 0.3
                                                 + state->speed_control.virtual_gas_pedal * 0.7;
    }
}

void speed_control_step(State *state) {
    static uint8_t pedal_control_was_enabled = false;
    static uint8_t car_was_connected = false;
    static int64_t gas_pedal_enable_time = 0;
    static double previous_target_value = 0; // Uses just to see if the current target has been updated

    if (state->speed_control.pedal_control.target_value != previous_target_value) {
        printf("target pedal: %lf\n", state->speed_control.pedal_control.target_value);
        previous_target_value = state->speed_control.pedal_control.target_value;
    }

    if (!cruise_control_safety_checks(state, car_was_connected)) {
        state->speed_control.pedal_control.enabled = false;
    }
    car_was_connected = state->car.is_connected;

    if (state->speed_control.pedal_control.enabled != pedal_control_was_enabled) {
        // Check if control was just now enabled
        if (state->speed_control.pedal_control.enabled) {
            state->speed_control.pedal_control.target_value = state->car.gas_pedal;
            state->speed_control.virtual_gas_pedal = state->car.gas_pedal;
            gas_pedal_enable_time = esp_timer_get_time_ms() + CAR_VIRTUAL_GAS_PEDAL_RISE_TIME_MS;

            printf("Pedal control enabled. \n"
                   "\tvirtual_gas_pedal = %lf\n",
                   state->speed_control.virtual_gas_pedal
            );
        } else {
            state->speed_control.pedal_control.previous_value = state->speed_control.pedal_control.target_value;
        }
    }
    pedal_control_was_enabled = state->speed_control.pedal_control.enabled;

    // Disable or enable gas pedal after pedal output rise time
    if (!state->speed_control.pedal_control.enabled) {
        // Disabling is done in control.c due to multiple speed controls wanting to control the pedal
    } else if (gas_pedal_enable_time == 0 || esp_timer_get_time_ms() > gas_pedal_enable_time) {
        gas_pedal_enable(true);
        // Reset time to 0 to prevent bugs when get_time_ms overflows
        gas_pedal_enable_time = 0;
    }

    pedal_control_apply_pid(state);

    gas_pedal_write(state);
}
