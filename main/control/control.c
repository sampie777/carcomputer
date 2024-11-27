//
// Created by samuel on 19-7-22.
//

#include <math.h>
#include "../config.h"
#include "control.h"
#include "../peripherals/canbus/canbus.h"
#include "../return_codes.h"
#include "../peripherals/buttons.h"
#include "../utils.h"
#include "../error_codes.h"
#include "../peripherals/led.h"
#include "../peripherals/gas_pedal.h"
#include "cruise_control.h"

void control_read_can_bus(State *state) {
    canbus_check_controller_connection(state);
    canbus_check_messages(state);

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
}

typedef enum {
    PidProportional = 0,
    PidIntegral,
    PidDerivative,
} PidIncreaseTarget;

void control_read_user_input(State *state) {
    static int64_t last_read_time = 0;
    static double pid_increase_step = 0.01;
    static PidIncreaseTarget pid_increase_target = PidProportional;

    if (esp_timer_get_time_ms() < last_read_time + BUTTONS_READ_INTERVAL_MS) return;
    last_read_time = esp_timer_get_time_ms();

    Button button = buttons_get_pressed();
    switch (button) {
        case BUTTON_NONE:
            break;
        case BUTTON_UP:
            printf("Button pressed: BUTTON_UP\n");
            state->cruise_control.enabled = true;
            break;
        case BUTTON_VOLUME_UP:
            printf("Button pressed: BUTTON_VOLUME_UP\n");
            if (!state->cruise_control.enabled) {
                if (pid_increase_target == PidProportional) {
                    state->cruise_control.pidKp += pid_increase_step;
                    printf("state->cruise_control.pidKp = %lf\n", state->cruise_control.pidKp);
                } else if (pid_increase_target == PidIntegral) {
                    state->cruise_control.pidKi += pid_increase_step;
                    printf("state->cruise_control.pidKi = %lf\n", state->cruise_control.pidKi);
                } else if (pid_increase_target == PidDerivative) {
                    state->cruise_control.pidKd += pid_increase_step;
                    printf("state->cruise_control.pidKd = %lf\n", state->cruise_control.pidKd);
                }
                break;
            }

            state->cruise_control.target_speed++;
            printf("target speed: %lf\n", state->cruise_control.target_speed);
            break;
        case BUTTON_VOLUME_DOWN:
            printf("Button pressed: BUTTON_VOLUME_DOWN\n");
            if (!state->cruise_control.enabled) {
                if (pid_increase_target == PidProportional) {
                    state->cruise_control.pidKp -= pid_increase_step;
                    printf("state->cruise_control.pidKp = %lf\n", state->cruise_control.pidKp);
                } else if (pid_increase_target == PidIntegral) {
                    state->cruise_control.pidKi -= pid_increase_step;
                    printf("state->cruise_control.pidKi = %lf\n", state->cruise_control.pidKi);
                } else if (pid_increase_target == PidDerivative) {
                    state->cruise_control.pidKd -= pid_increase_step;
                    printf("state->cruise_control.pidKd = %lf\n", state->cruise_control.pidKd);
                }
                break;
            }

            state->cruise_control.target_speed--;
            if (state->cruise_control.target_speed < 0) {
                state->cruise_control.target_speed = 0;
            }
            printf("target speed: %lf\n", state->cruise_control.target_speed);
            break;
        case BUTTON_SOURCE:
            printf("Button pressed: BUTTON_SOURCE\n");
            if (state->cruise_control.enabled) printf("Disconnecting cruise control because of user input\n");
            state->cruise_control.enabled = false;
            break;
        case BUTTON_SOURCE_LONG_PRESS:
            printf("Button pressed: BUTTON_SOURCE_LONG_PRESS\n");
            if (pid_increase_target == PidProportional) {
                pid_increase_target = PidIntegral;
                printf("pid_increase_target = PidIntegral\n");
            } else if (pid_increase_target == PidIntegral) {
                pid_increase_target = PidDerivative;
                printf("pid_increase_target = PidDerivative\n");
            } else {
                pid_increase_target = PidProportional;
                printf("pid_increase_target = PidProportional\n");
            }

        // utils_reboot(state);
            break;
        case BUTTON_INFO: printf("Button pressed: BUTTON_INFO\n");
            break;
        case BUTTON_DOWN: printf("Button pressed: BUTTON_DOWN\n");
            break;
        case BUTTON_VOLUME_UP_LONG_PRESS: printf("Button pressed: BUTTON_VOLUME_UP_LONG_PRESS\n");
            pid_increase_step *= 10;
            break;
        case BUTTON_VOLUME_DOWN_LONG_PRESS: printf("Button pressed: BUTTON_VOLUME_DOWN_LONG_PRESS\n");
            pid_increase_step *= 0.1;
            break;
        case BUTTON_INFO_LONG_PRESS: printf("Button pressed: BUTTON_INFO_LONG_PRESS\n");
            break;
        case BUTTON_UP_LONG_PRESS: printf("Button pressed: BUTTON_UP_LONG_PRESS\n");
            break;
        case BUTTON_DOWN_LONG_PRESS: printf("Button pressed: BUTTON_DOWN_LONG_PRESS\n");
            break;
        default:
            break;
    }
}

void control_led_indicator_step(State *state) {
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

void control_cruise_control(State *state) {
    cruise_control_step(state);
}

void control_init(State *state) {
    gpio_set_direction(POWER_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(CAR_ENGINE_SHUTOFF_DISABLE_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(CAR_CLAXON_PIN, GPIO_MODE_OUTPUT);

    led_init();

    canbus_init(state);
    gas_pedal_init(state, 0);
    buttons_init();
}

CarGearPosition estimate_car_gear(CarState *car) {
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

void control_car_gear(State *state) {
    state->car.estimated_gear = estimate_car_gear(&state->car);
}
