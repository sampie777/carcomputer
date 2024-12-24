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
    control_buttons_handle_pid_config(state, button);
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

void control_car_gear(State* state) {
    state->car.estimated_gear = estimate_car_gear(&state->car);
}

void control_mpu_power(State* state) {
    static int64_t ignition_off_time = 0;
    if (state->car.is_ignition_on) {
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
    state->power_off_count_down_sec = (int16_t) (remaining_ms / 1000);

    if (remaining_ms > 0 && esp_timer_get_time_ms() < ignition_off_time + POWER_OFF_MIN_TIMEOUT_MS) return;

    state->power_off_count_down_sec = 0;
    gpio_set_level(POWER_PIN, 0);
    delay_ms(5000); // Don't return to main loop but wait a bit till we die
    ignition_off_time = 0;
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
        sprintf(timestamp, "%04d-%02d-%02d'T'%02d:%02d:%02d.000%+d",
                time.year,
                time.month,
                time.day,
                time.hours,
                time.minutes,
                time.seconds,
                time.timezone);
    }

    printf("[LOG] control_crash_detection constructing message\n");
    sprintf(message, "CRASH! Location: %.5f,%.5f at %s (accuracy: %d%%). Force: %.1f g.",
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
