//
// Created by Samuel-Anton Jansen on 2025/11/07.
//


#define STEP (50)
#define RUN_TIME (10000)

#include "test_display.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_cruisecontrol.h"
#include "../utils.h"
#include "../peripherals/display/display.h"
#include "mocks/carcomputer/peripherals/display/sh1106_i2c.h"
#include "mocks/idf/esp_timer.h"
#include "utils/common.h"
#include "../control/cruise_control.h"
#include "../control/control.h"
#include "../control/buttons.h"
#include "mocks/idf/freertos/task.h"


void render_video(void) {
    int framerate = 1000 / STEP;
    char command[512];
    snprintf(command, sizeof(command),
             "ffmpeg -y -framerate %d -pattern_type glob -i '../../../test_output/*.bmp' -c:v libx264 -pix_fmt yuv420p '../../../test_output/out.mp4'",
             framerate);
    system(command);
}

State *_state;

void update_function() {
    display_update(_state);
    update_bitmap();
}

void test_display_cruisecontrol() {
    system("rm ../../../test_output/*.bmp");
    _esp_timer_set_time(1000 * 1000);

    State state = {0};
    _state = &state;
    state.boot.is_booting = false;
    state.power_off_count_down_sec = -1;
    state.cruise_control.pidKp = CRUISE_CONTROL_PID_Kp;
    state.cruise_control.pidKi = CRUISE_CONTROL_PID_Ki;
    state.cruise_control.pidKd = CRUISE_CONTROL_PID_Kd;
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

    state.car.speed = 50;
    state.car.gas_pedal = 0;
    state.cruise_control.enabled = true;
    cruise_control_step(&state);
    state.car.speed = 35;

    display_init();

    for (int i = 0; i < RUN_TIME / STEP; i++) {
        simulate_car_step(&state, STEP);

        control_cruise_control(&state);
        control_process_car(&state);

        update_function();
        step_time(STEP);
    }

    render_video();
}

void test_display_sms() {
    system("rm ../../../test_output/*.bmp");
    _esp_timer_set_time(1000 * 1000);

    State state = {0};
    _state = &state;
    state.boot.is_booting = false;
    state.power_off_count_down_sec = -1;
    state.device_name = "Default";
    state.car.gas_pedal_connected = true;
    state.car.is_connected = true;
    state.car.is_braking = false;
    state.car.estimated_gear = Gear1;
    state.car.is_parking_brake_on = false;

    state.display.current_screen = Screen_Actions;
    state.display.actions_option_selection = ScreenActionsOptions_ActivateSim;

    display_init();

    set_update_function(&update_function);

    for (int i = 0; i < RUN_TIME / STEP; i++) {
        if (i == 5) {
            control_buttons_handle(&state, BUTTON_UP);
            state.display.actions_option_selection = ScreenActionsOptions_ActivateDiagnostics;
        }

        simulate_car_step(&state, STEP);

        control_cruise_control(&state);
        control_process_car(&state);

        update_function();
        step_time(STEP);
    }

    render_video();
}

void test_display_aboutcar() {
    _esp_timer_set_time(1000 * 1000);
    State state = {
        .power_off_count_down_sec = -1,
        // .display.current_screen = Screen_About,
        // .display.subscreen.about = ScreenAbout_AboutCar,
        .car.is_ignition_on = true,
        .car.is_braking = true,
        .storage.is_connected = true,
        .car.speed = 100,
        .cruise_control.target_speed = 120,
        // .cruise_control.enabled = true,
        .cruise_control.virtual_gas_pedal = 0.3,

        // .boot = {
        //     .is_booting = true,
        //     .max_progress = 10,
        //     .progress = 3,
        // },
    };
    sprintf(state.storage.filename, "file.csv");

    display_init();
    SH1106Config *config = getConfig();

    display_update(&state);

    update_bitmap();
}

void test_display_lock_car() {
    system("rm ../../../test_output/*.bmp");
    _esp_timer_set_time(100 * 1000);
    State state = {
        .power_off_count_down_sec = -1,
        .car.is_ignition_on = true,
        .car.is_braking = true,
        .storage.is_connected = true,
        .car.speed = 100,
        .car.odometer_start = 100,
        .car.odometer = 110,
        .car.is_locked = false,
        .cruise_control.target_speed = 120,
        .cruise_control.virtual_gas_pedal = 0.3,
    };
    _state = &state;
    sprintf(state.storage.filename, "file.csv");

    display_init();

    for (int i = 0; i < RUN_TIME / STEP; i++) {
        simulate_car_step(&state, STEP);
        if (esp_timer_get_time_ms() == 7000) {
            state.car.odometer += 10;
        }

        if (esp_timer_get_time_ms() == 4000) {
            printf("Unlock car\n");
            state.car.is_locked = false;
        }
        if (esp_timer_get_time_ms() == 3000) {
            printf("Lock car\n");
            state.car.is_locked = true;
        }

        control_manage_car_lock(&state);
        control_cruise_control(&state);
        control_process_car(&state);

        update_function();
        step_time(STEP);
    }

    render_video();
}
