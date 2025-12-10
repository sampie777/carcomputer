//
// Created by samuel on 19-7-22.
//

#include <math.h>
#include <driver/gpio.h>
#include "../config.h"
#include "control.h"
#include "buttons.h"
#include "../peripherals/canbus/canbus.h"
#include "../peripherals/mpu9250.h"
#include "../return_codes.h"
#include "../utils.h"
#include "../error_codes.h"
#include "../peripherals/gas_pedal.h"
#include "cruise_control.h"
#include "speed_control.h"

void control_read_can_bus(State *state) {
    canbus_check_controller_connection(state);
    if (state->car.is_controller_connected) {
        canbus_check_messages(state);
    }

    if (esp_timer_get_time_ms() < state->car.last_can_message_time + CAR_CAN_MAX_MESSAGE_RECEIVE_TIMEOUT) {
        state->car.is_connected = true;
    } else {
        state->car.is_connected = false;
    }
}

void control_read_analog_sensors(State *state) {
    if (gas_pedal_read(state) == RESULT_DISCONNECTED) {
        set_error(state, ERROR_PEDAL_DISCONNECTED);
    }

    mpu9250_read(state);
}

void control_read_user_input(State *state) {
    static int64_t last_read_time = 0;

    if (esp_timer_get_time_ms() < last_read_time + BUTTONS_READ_INTERVAL_MS) return;
    last_read_time = esp_timer_get_time_ms();

    Button button = buttons_get_pressed(&state->buttons, &state->motion);
    control_buttons_handle(state, button);
}

void control_cruise_control(State *state) {
    cruise_control_step(state);
    speed_control_step(state);

    if (!state->speed_control.cruise_control.enabled && !state->speed_control.pedal_control.enabled) {
        gas_pedal_enable(false);
    }

    if (!state->speed_control.cruise_control.enabled) {
        state->display.subscreen.cruise_control = 0;
    }
}

void control_init(State *state) {
    gpio_set_direction(POWER_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(CAR_ENGINE_SHUTOFF_DISABLE_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(CAR_CLAXON_PIN, GPIO_MODE_OUTPUT);

    canbus_init();
    gas_pedal_init(state, 0);
    buttons_init();
    mpu9250_init(state);
}

CarGearPosition estimate_car_gear(CarState *car) {
    static double ratio = 0;

    if (car->is_in_reverse) {
        return GearReverse;
    }

    if (car->rpm == 0) {
        return GearNeutral;
    }

    ratio = ratio * 0.8 + 0.2 * (car->speed / car->rpm_raw * 10000);
    if (ratio > CAR_GEAR_1_RATIO - CAR_GEAR_RATIO_SLACK && ratio < CAR_GEAR_1_RATIO + CAR_GEAR_RATIO_SLACK)
        return Gear1;
    if (ratio > CAR_GEAR_2_RATIO - CAR_GEAR_RATIO_SLACK && ratio < CAR_GEAR_2_RATIO + CAR_GEAR_RATIO_SLACK)
        return Gear2;
    if (ratio > CAR_GEAR_3_RATIO - CAR_GEAR_RATIO_SLACK && ratio < CAR_GEAR_3_RATIO + CAR_GEAR_RATIO_SLACK)
        return Gear3;
    if (ratio > CAR_GEAR_4_RATIO - CAR_GEAR_RATIO_SLACK && ratio < CAR_GEAR_4_RATIO + CAR_GEAR_RATIO_SLACK)
        return Gear4;
    if (ratio > CAR_GEAR_5_RATIO - CAR_GEAR_RATIO_SLACK && ratio < CAR_GEAR_5_RATIO + CAR_GEAR_RATIO_SLACK)
        return Gear5;
    return GearNeutral;
}

void calculate_acceleration(CarState *car) {
    static int64_t last_speed_update_time = 0;
    static double last_speed_ms = -999;

    int64_t current_time = esp_timer_get_time_ms();
    if (current_time < last_speed_update_time + CAR_SPEED_AVERAGE_PERIOD_MS) return;

    double current_speed_ms = car->speed * 1000.0 / 3600.0; // Convert km/h to m/s
    if (last_speed_ms < -900) last_speed_ms = current_speed_ms;

    double speed_difference = current_speed_ms - last_speed_ms;
    car->acceleration = speed_difference * 0.2 + car->acceleration * 0.8;

    last_speed_update_time = current_time;
    last_speed_ms = current_speed_ms;
}

void control_process_car(State *state) {
    state->car.estimated_gear = estimate_car_gear(&state->car);
    calculate_acceleration(&state->car);
}

void control_mpu_power(State *state) {
    static int64_t ignition_off_time = 0;
    if (!state->boot.is_rebooting && (
            state->car.is_ignition_on ||
            state->diagnostics.status != DiagnosticsStep_Off || // Whether diagnostics is running
            !state->car.is_controller_connected                 // Don't power off the module as we are most probably not in a car environment
        )) {
        gpio_set_level(POWER_PIN, 1);
        ignition_off_time = 0;
        state->power_off_count_down_sec = -1;
        return;
    }

    if (ignition_off_time == 0) {
        ignition_off_time = esp_timer_get_time_ms();
    }

    state->speed_control.cruise_control.enabled = false;
    long remaining_ms = (long) (ignition_off_time + POWER_OFF_MIN_TIMEOUT_MS - esp_timer_get_time_ms());
    state->power_off_count_down_sec = max(0, (int16_t) ceil(remaining_ms / 1000.0));

    if (remaining_ms > 0 && esp_timer_get_time_ms() < ignition_off_time + POWER_OFF_MIN_TIMEOUT_MS) return;

    state->power_off_count_down_sec = 0;

    if (state->boot.is_rebooting) {
        esp_restart();
    } else {
        gpio_set_level(POWER_PIN, 0);
    }
}

void control_crash_detection(State *state) {
    static int64_t last_sent = 0;
    double total_force = sqrt(
        state->motion.accel_x * state->motion.accel_x
        + state->motion.accel_y * state->motion.accel_y
        + state->motion.accel_z * state->motion.accel_z
    );
    if (total_force < CRASH_DETECTION_CRASH_MIN_G) return;

    if (esp_timer_get_time_ms() < last_sent + CRASH_DETECTION_CRASH_MAX_DURATION_MS) return;
    last_sent = esp_timer_get_time_ms();

    printf("[LOG] control_crash_detection crash detected\n");

#ifdef ICE_CONTACT_NUMBER
    char message[158]; // Max SMS length

    set_error(state, ERROR_CRASH_DETECTED);

    char timestamp[64];
    Time time = state->location.time.year > 2021 ? state->location.time : state->gsm.time;
    if (time.year < 2000) {
        timestamp[0] = '\0';
    } else {
        snprintf(timestamp, sizeof timestamp, "%04d-%02d-%02d'T'%02d:%02d:%02d.000%+d",
                 time.year,
                 time.month,
                 time.day,
                 time.hours,
                 time.minutes,
                 time.seconds,
                 time.timezone);
    }

    printf("[LOG] control_crash_detection constructing message\n");
    snprintf(message, sizeof message, "CRASH! Location: %.5f,%.5f at %s (accuracy: %d%%). Force: %.1f g.",
             state->location.latitude,
             state->location.longitude,
             timestamp,
             state->location.satellites / 4 * 100,
             total_force
    );

    // Loop over all specified numbers and send them
    char numbers[] = ICE_CONTACT_NUMBER;
    char *number = strtok(numbers, ";");
    while (number != NULL) {
        printf("[LOG] control_crash_detection sending message\n");
        gsm_send_sms(number, message);
        number = strtok(NULL, ";");
    }
#else
    set_error(state, ERROR_CRASH_NO_ICE);
#endif
}

void control_run_diagnostics_activation(State *state) {
    static DiagnosticsStepStatus previous_status = DiagnosticsStep_Off;
    static int64_t wait_timer_end = 0;
    static int64_t press_timer_start = 0;
    static int8_t depressed_count = 0;

    if (state->diagnostics.status != previous_status) {
        if (state->diagnostics.status == DiagnosticsStep_Off) {
            gas_pedal_enable(false);
            state->speed_control.virtual_gas_pedal = 0;
            // Go back to Actions screen
            state->display.current_screen = Screen_Actions;
        } else if (previous_status == DiagnosticsStep_Off) {
            // Start of a new process cycle
            state->display.current_screen = Screen_ActivateDiagnostics;
        }

        wait_timer_end = 0;
        press_timer_start = 0;
        depressed_count = 0;
        previous_status = state->diagnostics.status;
    }

    if (state->diagnostics.status == DiagnosticsStep_Off) {
        return;
    }

    // Init checks
    if (!state->car.is_connected) {
        printf("[Diagnostics] Car not connected\n");
        set_error(state, ERROR_CAR_DISCONNECTED);
        state->diagnostics.status = DiagnosticsStep_Off;
        return;
    }

    if (!state->car.is_controller_connected) {
        printf("[Diagnostics] Car controller not connected\n");
        set_error(state, ERROR_CAR_DISCONNECTED);
        state->diagnostics.status = DiagnosticsStep_Off;
        return;
    }

    if (!state->car.gas_pedal_connected) {
        printf("[Diagnostics] Gas pedal not connected\n");
        set_error(state, ERROR_PEDAL_DISCONNECTED);
        state->diagnostics.status = DiagnosticsStep_Off;
        return;
    }

    // Check if ignition is off before going to the next state
    if (state->diagnostics.status == DiagnosticsStep_IgnitionOff) {
        state->speed_control.virtual_gas_pedal = 0;
        gas_pedal_write(state);
        gas_pedal_enable(true);

        if (state->car.is_ignition_on) return;
        if (state->car.speed > 0) return;
        if (state->car.rpm > 0) return;
        if (!state->car.is_parking_brake_on) return;
        if (state->car.is_braking) return;

        state->diagnostics.status++;
        return;
    }

    if (state->diagnostics.status == DiagnosticsStep_IgnitionOffWait5Sec) {
        if (wait_timer_end <= 0) {
            wait_timer_end = esp_timer_get_time_ms() + DIAGNOSTICS_0_WAIT5SEC_MS;
        }

        if (esp_timer_get_time_ms() >= wait_timer_end) {
            state->diagnostics.status++;
            return;
        }

        if (state->car.is_ignition_on) {
            printf("[Diagnostics] Ignition unexpectedly turned on\n");
            state->diagnostics.status = DiagnosticsStep_IgnitionOff;
            return;
        }
        return;
    }

    // Put car in reset/boot mode
    if (state->diagnostics.status == DiagnosticsStep_IgnitionOn) {
        if (!state->car.is_ignition_on) return;

        state->diagnostics.status++;
        state->diagnostics.process_start_time = esp_timer_get_time_ms();
        state->diagnostics.process_estimated_end_time = state->diagnostics.process_start_time +
                                                        DIAGNOSTICS_1_WAIT3SEC_MS +
                                                        DIAGNOSTICS_2_DEPRESS_PEDAL_COUNT * 2 * DIAGNOSTICS_2_DEPRESS_PEDAL_INTERVAL +
                                                        DIAGNOSTICS_3_WAIT7SEC_MS +
                                                        DIAGNOSTICS_4_DEPRESS_PEDAL_FULLY_TIME;

        return;
    }

    // Safety checks
    if (!state->car.is_ignition_on) {
        printf("[Diagnostics] Ignition unexpectedly turned off\n");
        state->diagnostics.status = DiagnosticsStep_IgnitionOff;
        return;
    }

    if (state->car.rpm > 0) {
        printf("[Diagnostics] Engine unexpectedly turned on\n");
        state->diagnostics.status = DiagnosticsStep_IgnitionOff;
        return;
    }

    // Continue normal procedure
    if (state->diagnostics.status == DiagnosticsStep_IgnitionOnWait3Sec) {
        if (wait_timer_end <= 0) {
            wait_timer_end = esp_timer_get_time_ms() + DIAGNOSTICS_1_WAIT3SEC_MS;
        }

        if (esp_timer_get_time_ms() >= wait_timer_end) {
            state->diagnostics.status++;
            return;
        }
    }

    if (state->diagnostics.status == DiagnosticsStep_DepressPedal5Times) {
        if (depressed_count >= DIAGNOSTICS_2_DEPRESS_PEDAL_COUNT) {
            state->speed_control.virtual_gas_pedal = 0;
            gas_pedal_write(state);
            state->diagnostics.status++;
            return;
        }

        if (esp_timer_get_time_ms() < press_timer_start + DIAGNOSTICS_2_DEPRESS_PEDAL_INTERVAL) return;
        press_timer_start = esp_timer_get_time_ms();

        if (state->speed_control.virtual_gas_pedal < 0.5) {
            printf("[Diagnostics] Pedal in...\n");
            state->speed_control.virtual_gas_pedal = 1;
        } else {
            printf("[Diagnostics] Pedal out...\n");
            state->speed_control.virtual_gas_pedal = 0;
            depressed_count++;
        }
        gas_pedal_write(state);
    }

    if (state->diagnostics.status == DiagnosticsStep_OnWait7Sec) {
        if (wait_timer_end <= 0) {
            wait_timer_end = esp_timer_get_time_ms() + DIAGNOSTICS_3_WAIT7SEC_MS;
        }

        if (esp_timer_get_time_ms() >= wait_timer_end) {
            state->diagnostics.status++;
            return;
        }
    }

    if (state->diagnostics.status == DiagnosticsStep_DepressPedal10Sec) {
        if (wait_timer_end <= 0) {
            state->speed_control.virtual_gas_pedal = 1;
            gas_pedal_write(state);
            // Add some extra time (1000 ms) to make sure
            wait_timer_end = esp_timer_get_time_ms() + DIAGNOSTICS_4_DEPRESS_PEDAL_FULLY_TIME + 1000;
        }

        if (esp_timer_get_time_ms() >= wait_timer_end) {
            state->diagnostics.status++;
            return;
        }
    }

    if (state->diagnostics.status == DiagnosticsStep_ReleasePedal) {
        if (wait_timer_end <= 0) {
            wait_timer_end = esp_timer_get_time_ms() + 3000;
            state->speed_control.virtual_gas_pedal = 0;
            gas_pedal_write(state);
            gas_pedal_enable(false);
        }

        if (esp_timer_get_time_ms() >= wait_timer_end) {
            state->diagnostics.status = DiagnosticsStep_Off;
            return;
        }
    }
}

void control_manage_car_lock(State *state) {
    static uint32_t last_unlocked_odometer = 0;
    static bool was_locked = false;
    state->car.should_be_locked = false;

    if ((!state->car.is_locked && was_locked) || (last_unlocked_odometer == 0 && state->car.odometer > 0)) {
        last_unlocked_odometer = state->car.odometer;
    }
    was_locked = state->car.is_locked;

    if (state->car.is_locked) return;
    if (state->car.speed < 30) return;
    // Dismiss alert after 3 km of driving
    if (state->car.odometer_start > 0 && state->car.odometer - last_unlocked_odometer > 3) return;

    state->car.should_be_locked = true;
}
