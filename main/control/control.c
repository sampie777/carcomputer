//
// Created by samuel on 19-7-22.
//

#include <math.h>
#include "../config.h"
#include "control.h"

#include "buttons.h"
#include "../peripherals/canbus/canbus.h"
#include "../peripherals/mpu9250.h"
#include "../return_codes.h"
#include "../peripherals/buttons.h"
#include "../utils.h"
#include "../error_codes.h"
#include "../peripherals/led.h"
#include "../peripherals/gas_pedal.h"
#include "cruise_control.h"

void control_read_can_bus(State* state) {
    canbus_check_controller_connection(state);
    canbus_check_messages(state);

    if (esp_timer_get_time_ms() < state->car.last_can_message_time + CAR_CAN_MAX_MESSAGE_RECEIVE_TIMEOUT) {
        state->car.is_connected = true;
    } else {
        state->car.is_connected = false;
    }
}

void control_read_analog_sensors(State* state) {
    if (gas_pedal_read(state) == RESULT_DISCONNECTED) {
        set_error(state, ERROR_PEDAL_DISCONNECTED);
    }

    mpu9250_read(state);
}


void control_read_user_input(State* state) {
    static int64_t last_read_time = 0;

    if (esp_timer_get_time_ms() < last_read_time + BUTTONS_READ_INTERVAL_MS) return;
    last_read_time = esp_timer_get_time_ms();

    Button button = buttons_get_pressed();
    control_buttons_handle(state, button);
//    control_buttons_handle_pid_config(state, button);
}

void control_led_indicator_step(State* state) {
    if (state->is_booting) {
        led_blink(1500);
        return;
    }
    if (!state->car.is_connected) {
        led_blink(800);
        return;
    }
    if (!state->car.gas_pedal_connected) {
        led_blink(300);
        return;
    }

    led_set(state->cruise_control.enabled);
}

void control_cruise_control(State* state) {
    cruise_control_step(state);
}

void control_init(State* state) {
    gpio_set_direction(POWER_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(CAR_ENGINE_SHUTOFF_DISABLE_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(CAR_CLAXON_PIN, GPIO_MODE_OUTPUT);

    led_init();

    canbus_init(state);
    gas_pedal_init(state, 0);
    buttons_init();
    mpu9250_init();
}

CarGearPosition estimate_car_gear(CarState* car) {
    if (car->is_in_reverse) {
        return GearReverse;
    }

    if (car->rpm == 0) {
        return GearNeutral;
    }

    double ratio = car->speed / car->rpm_raw * 10000;
    int rounded_ration = (int) round(ratio);
    switch (rounded_ration) {
        case CAR_GEAR_1_RATIO:
            return Gear1;
        case CAR_GEAR_2_RATIO:
            return Gear2;
        case CAR_GEAR_3_RATIO:
            return Gear3;
        case CAR_GEAR_4_RATIO:
            return Gear4;
        case CAR_GEAR_5_RATIO:
            return Gear5;
        default:
            return GearNeutral;
    }
}

void calculate_acceleration(CarState* car) {
    static int64_t last_speed_update_time = 0;
    static double last_speed = 0.0;

    int64_t current_time = esp_timer_get_time_ms();
    if (current_time < last_speed_update_time + 80) return;

    double current_speed_m_s = car->speed * 1000.0 / 3600.0; // Convert km/h to m/s
    double speed_difference = current_speed_m_s - last_speed;
    double time_difference = (current_time - last_speed_update_time) / 1000.0;
    double acceleration = time_difference == 0 ? 0 : speed_difference / time_difference;

    car->acceleration = (car->acceleration * 0.75) + (acceleration * 0.25);

    last_speed_update_time = current_time;
    last_speed = current_speed_m_s;
}

void control_process_car(State* state) {
    state->car.estimated_gear = estimate_car_gear(&state->car);
    calculate_acceleration(&state->car);
}

void control_mpu_power(State* state) {
    static int64_t ignition_off_time = 0;
    if (!state->is_rebooting && (state->car.is_ignition_on || state->error_codes.status != ErrorCodes_Off)) {
        gpio_set_level(POWER_PIN, 1);
        ignition_off_time = 0;
        state->power_off_count_down_sec = -1;
        return;
    }

    if (ignition_off_time == 0) {
        ignition_off_time = esp_timer_get_time_ms();
    }

    state->cruise_control.enabled = false;
    long remaining_ms = (long) (ignition_off_time + POWER_OFF_MIN_TIMEOUT_MS - esp_timer_get_time_ms());
    state->power_off_count_down_sec = max(0, (int16_t) ceil(remaining_ms / 1000.0));

    if (remaining_ms > 0 && esp_timer_get_time_ms() < ignition_off_time + POWER_OFF_MIN_TIMEOUT_MS) return;

    state->power_off_count_down_sec = 0;

    if (state->is_rebooting) {
        esp_restart();
    } else {
        gpio_set_level(POWER_PIN, 0);
    }
}

void control_crash_detection(State* state) {
    static int64_t last_sent = 0;
    double total_force = sqrt(
        state->motion.accel_x * state->motion.accel_x + state->motion.accel_y * state->motion.accel_y + state->motion.
        accel_z * state->motion.accel_z);
    if (total_force < CRASH_DETECTION_CRASH_MIN_G) return;

    if (esp_timer_get_time_ms() < last_sent + CRASH_DETECTION_CRASH_MAX_DURATION_MS) return;
    last_sent = esp_timer_get_time_ms();

    printf("[LOG] control_crash_detection crash detected\n");

#ifdef ICE_CONTACT_NUMBER
    char message[158];   // Max SMS length

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

void control_read_error_codes(State* state) {
    static ErrorCodesStatus previous_status = ErrorCodes_Off;
    static int64_t wait_timer_end = 0;
    static int64_t press_timer_start = 0;
    static int8_t depressed_count = 0;

    if (state->error_codes.status != previous_status) {
        if (state->error_codes.status == ErrorCodes_Off) {
            gas_pedal_enable(false);
            state->cruise_control.virtual_gas_pedal = 0;
            // Go back to Actions screen
            state->display.current_screen = Screen_Actions;
        } else if (previous_status == ErrorCodes_Off) {
            // Start of a new process cycle
            state->display.current_screen = Screen_ErrorCodes;
        }

        wait_timer_end = 0;
        press_timer_start = 0;
        depressed_count = 0;
        previous_status = state->error_codes.status;
    }

    if (state->error_codes.status == ErrorCodes_Off) {
        return;
    }

    // Init checks
    if (!state->car.is_connected) {
        printf("[ErrorCodes] Car not connected\n");
        set_error(state, ERROR_CAR_DISCONNECTED);
        state->error_codes.status = ErrorCodes_Off;
        return;
    }

    if (!state->car.is_controller_connected) {
        printf("[ErrorCodes] Car controller not connected\n");
        set_error(state, ERROR_CAR_DISCONNECTED);
        state->error_codes.status = ErrorCodes_Off;
        return;
    }

    if (!state->car.gas_pedal_connected) {
        printf("[ErrorCodes] Gas pedal not connected\n");
        set_error(state, ERROR_PEDAL_DISCONNECTED);
        state->error_codes.status = ErrorCodes_Off;
        return;
    }

    // Check if ignition is off before going to the next state
    if (state->error_codes.status == ErrorCodes_IgnitionOff) {
        state->cruise_control.virtual_gas_pedal = 0;
        gas_pedal_write(state);
        gas_pedal_enable(true);

        if (state->car.is_ignition_on) return;
        if (state->car.speed > 0) return;
        if (state->car.rpm > 0) return;
        if (!state->car.is_parking_brake_on) return;
        if (state->car.is_braking) return;

        state->error_codes.status++;
        return;
    }

    if (state->error_codes.status == ErrorCodes_IgnitionOffWait5Sec) {
        if (wait_timer_end <= 0) {
            wait_timer_end = esp_timer_get_time_ms() + ERROR_CODES_0_WAIT5SEC_MS;
        }

        if (esp_timer_get_time_ms() >= wait_timer_end) {
            state->error_codes.status++;
            return;
        }

        if (state->car.is_ignition_on) {
            printf("[ErrorCodes] Ignition unexpectedly turned on\n");
            state->error_codes.status = ErrorCodes_IgnitionOff;
            return;
        }
        return;
    }

    // Put car in reset/boot mode
    if (state->error_codes.status == ErrorCodes_IgnitionOn) {
        if (!state->car.is_ignition_on) return;

        state->error_codes.status++;
        state->error_codes.process_start_time = esp_timer_get_time_ms();
        state->error_codes.process_estimated_end_time = state->error_codes.process_start_time +
            ERROR_CODES_1_WAIT3SEC_MS +
            ERROR_CODES_2_DEPRESS_PEDAL_COUNT * 2 * ERROR_CODES_2_DEPRESS_PEDAL_INTERVAL +
            ERROR_CODES_3_WAIT7SEC_MS +
            ERROR_CODES_4_DEPRESS_PEDAL_FULLY_TIME;

        return;
    }

    // Safety checks
    if (!state->car.is_ignition_on) {
        printf("[ErrorCodes] Ignition unexpectedly turned off\n");
        state->error_codes.status = ErrorCodes_IgnitionOff;
        return;
    }

    if (state->car.rpm > 0) {
        printf("[ErrorCodes] Engine unexpectedly turned on\n");
        state->error_codes.status = ErrorCodes_IgnitionOff;
        return;
    }

    // Continue normal procedure
    if (state->error_codes.status == ErrorCodes_IgnitionOnWait3Sec) {
        if (wait_timer_end <= 0) {
            wait_timer_end = esp_timer_get_time_ms() + ERROR_CODES_1_WAIT3SEC_MS;
        }

        if (esp_timer_get_time_ms() >= wait_timer_end) {
            state->error_codes.status++;
            return;
        }
    }

    if (state->error_codes.status == ErrorCodes_DepressPedal5Times) {
        if (depressed_count >= ERROR_CODES_2_DEPRESS_PEDAL_COUNT) {
            state->cruise_control.virtual_gas_pedal = 0;
            gas_pedal_write(state);
            state->error_codes.status++;
            return;
        }

        if (esp_timer_get_time_ms() < press_timer_start + ERROR_CODES_2_DEPRESS_PEDAL_INTERVAL) return;
        press_timer_start = esp_timer_get_time_ms();

        if (state->cruise_control.virtual_gas_pedal < 0.5) {
            printf("[ErrorCodes] Pedal in...\n");
            state->cruise_control.virtual_gas_pedal = 1;
        } else {
            printf("[ErrorCodes] Pedal out...\n");
            state->cruise_control.virtual_gas_pedal = 0;
            depressed_count++;
        }
        gas_pedal_write(state);
    }

    if (state->error_codes.status == ErrorCodes_OnWait7Sec) {
        if (wait_timer_end <= 0) {
            wait_timer_end = esp_timer_get_time_ms() + ERROR_CODES_3_WAIT7SEC_MS;
        }

        if (esp_timer_get_time_ms() >= wait_timer_end) {
            state->error_codes.status++;
            return;
        }
    }

    if (state->error_codes.status == ErrorCodes_DepressPedal10Sec) {
        if (wait_timer_end <= 0) {
            state->cruise_control.virtual_gas_pedal = 1;
            gas_pedal_write(state);
            // Add some extra time (1000 ms) to make sure
            wait_timer_end = esp_timer_get_time_ms() + ERROR_CODES_4_DEPRESS_PEDAL_FULLY_TIME + 1000;
        }

        if (esp_timer_get_time_ms() >= wait_timer_end) {
            state->error_codes.status++;
            return;
        }
    }

    if (state->error_codes.status == ErrorCodes_ReleasePedal) {
        if (wait_timer_end <= 0) {
            wait_timer_end = esp_timer_get_time_ms() + 3000;
            state->cruise_control.virtual_gas_pedal = 0;
            gas_pedal_write(state);
            gas_pedal_enable(false);
        }

        if (esp_timer_get_time_ms() >= wait_timer_end) {
            state->error_codes.status = ErrorCodes_Off;
            return;
        }
    }
}
