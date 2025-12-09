//
// Created by Samuel-Anton Jansen on 2025/12/08.
//

#include "test_cc_settings.h"

#define STEP (50)
#define RUN_TIME (10000)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_cruisecontrol.h"
#include "../utils.h"
#include "../peripherals/display/display.h"
#include "mocks/carcomputer/peripherals/display/sh1106_i2c.h"
#include "utils/common.h"
#include "../control/control.h"
#include "../control/buttons.h"
#include "utils/video.h"

bool next_button(int loop) {
    static int index = 0;
    static int last_executed_index = -1;
    static int last_time = 300;
    static int last_loop = -1;

    // Detect loop iteration start
    if (last_loop < loop) index = 0;
    last_loop = loop;

    // Only execute each 200 ms
    if (index == 0 && last_time + 200 > esp_timer_get_time_ms()) return false;
    last_time = esp_timer_get_time_ms();

    if (index == last_executed_index + 1) {
        last_executed_index = index;
        index = 0;  // Prevent further iterations
        return true;
    }

    index++;
    return false;
}

void test_cc_settings_screen() {
    State state = {0};
    video_init(&state);
    state.boot.is_booting = false;
    state.power_off_count_down_sec = -1;
    state.speed_control.cruise_control.pidKp = CRUISE_CONTROL_PID_Kp;
    state.speed_control.cruise_control.pidKi = CRUISE_CONTROL_PID_Ki;
    state.speed_control.cruise_control.pidKd = CRUISE_CONTROL_PID_Kd;
    state.device_name = "Default";
    state.location.time.timezone = 2; // GMT+2
    state.motion.bias.x = 1.1;
    state.motion.bias.y = -0.04;
    state.motion.bias.z = 0.01;
    state.a9g.initialized = true;
    state.location.is_gps_on = true;

    state.car.gas_pedal_connected = true;
    state.car.is_connected = true;
    state.car.is_braking = false;
    state.car.estimated_gear = Gear1;
    state.car.is_parking_brake_on = false;

    display_init();

    for (int i = 0; i < RUN_TIME / STEP; i++) {
        if (next_button(i)) control_buttons_handle(&state, BUTTON_CANCEL);  // Go to main menu
        if (next_button(i)) control_buttons_handle(&state, BUTTON_DECREASE);
        if (next_button(i)) control_buttons_handle(&state, BUTTON_DECREASE);
        if (next_button(i)) control_buttons_handle(&state, BUTTON_CONFIRM); // Actions menu
        if (next_button(i)) control_buttons_handle(&state, BUTTON_INCREASE);
        if (next_button(i)) control_buttons_handle(&state, BUTTON_CONFIRM);
        if (next_button(i)) control_buttons_handle(&state, BUTTON_CANCEL);
        if (next_button(i)) control_buttons_handle(&state, BUTTON_CONFIRM);

        // Edit second value
        if (next_button(i)) control_buttons_handle(&state, BUTTON_DECREASE);
        if (next_button(i)) control_buttons_handle(&state, BUTTON_CONFIRM);
        if (next_button(i)) control_buttons_handle(&state, BUTTON_INCREASE);   // Increase factor
        if (next_button(i)) control_buttons_handle(&state, BUTTON_CONFIRM);    // Go to apply value
        if (next_button(i)) control_buttons_handle(&state, BUTTON_INCREASE);   // Add factor to value
        if (next_button(i)) control_buttons_handle(&state, BUTTON_INCREASE);   // Add factor to value
        if (next_button(i)) control_buttons_handle(&state, BUTTON_CANCEL);   // Edit factor
        if (next_button(i)) control_buttons_handle(&state, BUTTON_DECREASE);
        if (next_button(i)) control_buttons_handle(&state, BUTTON_DECREASE);
        if (next_button(i)) control_buttons_handle(&state, BUTTON_DECREASE);
        if (next_button(i)) control_buttons_handle(&state, BUTTON_CONFIRM);    // Go to apply value
        if (next_button(i)) control_buttons_handle(&state, BUTTON_DECREASE);
        if (next_button(i)) control_buttons_handle(&state, BUTTON_DECREASE);
        if (next_button(i)) control_buttons_handle(&state, BUTTON_CANCEL);   // Edit factor
        if (next_button(i)) control_buttons_handle(&state, BUTTON_CANCEL);   // Go to selection screen

        // Edit speed limiter
        if (next_button(i)) control_buttons_handle(&state, BUTTON_DECREASE);
        if (next_button(i)) control_buttons_handle(&state, BUTTON_DECREASE);
        if (next_button(i)) control_buttons_handle(&state, BUTTON_CONFIRM); // Toggle
        if (next_button(i)) control_buttons_handle(&state, BUTTON_DECREASE);

        simulate_car_step(&state, STEP);

        control_cruise_control(&state);
        control_process_car(&state);

        video_display_update();
        step_time(STEP);
    }

    video_render(1000 / STEP);
    // video_init(&state);
    // video_display_update();
}
