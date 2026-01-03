//
// Created by Samuel-Anton Jansen on 2025/11/07.
//

#include "test_cruisecontrol.h"
#include <stdio.h>

#include "../control/cruise_control.h"
#include "../utils.h"
#include "../control/buttons.h"
#include "../control/control.h"
#include "mocks/idf/esp_timer.h"
#include "mocks/carcomputer/peripherals/gas_pedal.h"
#include "utils/common.h"
#include "utils/csv.h"
#include "utils/graph.h"

#define STEP (100)
#define RUN_TIME (30000)

void test_cruise_control(void) {
    write_csv();
    double max_speed = 0;
    double min_speed = 100;

    State state = {0};
    state.boot.is_booting = true;
    state.power_off_count_down_sec = -1;
    state.speed_control.cruise_control.pidKp = CRUISE_CONTROL_PID_Kp;
    state.speed_control.cruise_control.pidKi = CRUISE_CONTROL_PID_Ki;
    state.speed_control.cruise_control.pidKd = CRUISE_CONTROL_PID_Kd;
    state.speed_control.cruise_control.speed_limiter_type = CruiseControlSpeedLimiterType_Proportional;
    state.device_name = "Default";
    state.location.time.timezone = 2; // GMT+2
    state.motion.bias.x = 1.1;
    state.motion.bias.y = -0.04;
    state.motion.bias.z = 0.01;

    state.car.gas_pedal_connected = true;
    state.car.is_connected = true;
    state.car.is_braking = false;
    state.car.estimated_gear = Gear1;
    state.car.is_parking_brake_on = false;

    state.car.speed = 50;
    state.car.gas_pedal = 0;
    cruise_control_step(&state);
    state.car.speed = 40;

    Graph speed_graph = {.max = 55, .min = 30, .highlight_y = 50, .size = 0};
    Graph gas_pedal_graph = {.max = 1, .min = 0, .size = 0};
    Graph acceleration_graph = {
        .max = 0.5, .min = -0.1, .size = 0,
        .highlight_y = CRUISE_CONTROL_MAX_ACCELERATION_MS2_LOWER_BOUND
    };

    for (int i = 0; i < RUN_TIME / STEP; i++) {
        if (esp_timer_get_time_ms() == 3000) {
            state.speed_control.cruise_control.enabled = true;
        }
        state.speed_control.cruise_control.target_speed = 50;

        simulate_car_step(&state, STEP);

        control_cruise_control(&state);
        control_process_car(&state);

        graph_add(&speed_graph, state.car.speed);
        graph_add(&gas_pedal_graph, state.speed_control.virtual_gas_pedal);
        graph_add(&acceleration_graph, state.car.acceleration);

        step_time(STEP);

        max_speed = max(max_speed, state.car.speed);
        min_speed = min(min_speed, state.car.speed);
    }

    graph_render(&speed_graph, "../../../test_output/speed_graph.bmp");
    graph_render(&gas_pedal_graph, "../../../test_output/gas_pedal_graph.bmp");
    graph_render(&acceleration_graph, "../../../test_output/acceleration_graph.bmp");
    printf("Max speed: %lf\n", max_speed);
    printf("Min speed: %lf\n", min_speed);
}

void test_speed_control() {
    write_csv();
    double max_speed = 0;
    double min_speed = 100;

    State state = {0};
    state.boot.is_booting = false;
    state.power_off_count_down_sec = -1;
    state.device_name = "Default";
    state.location.time.timezone = 2; // GMT+2
    state.motion.bias.x = 1.1;
    state.motion.bias.y = -0.04;
    state.motion.bias.z = 0.01;
    state.display.current_screen = Screen_Speed;

    state.car.gas_pedal_connected = true;
    state.car.is_connected = true;
    state.car.is_braking = false;
    state.car.estimated_gear = Gear1;
    state.car.is_parking_brake_on = false;
    state.car.gas_pedal = 0.3;

    Graph speed_graph = {.max = 80, .min = 0, .highlight_y = 50, .size = 0};
    Graph gas_pedal_graph = {.max = 1, .min = 0, .size = 0};
    Graph acceleration_graph = {
        .max = 0.5, .min = -0.1, .size = 0,
        .highlight_y = CRUISE_CONTROL_MAX_ACCELERATION_MS2_LOWER_BOUND
    };

    for (int i = 0; i < RUN_TIME / STEP; i++) {
        if (esp_timer_get_time_ms() == 3000) state.speed_control.pedal_control.enabled = true;
        if (esp_timer_get_time_ms() == 4200) state.car.gas_pedal *= 0.75;
        if (esp_timer_get_time_ms() == 4500) state.car.gas_pedal *= 0.5;
        if (esp_timer_get_time_ms() == 4700) state.car.gas_pedal *= 0.5;
        if (esp_timer_get_time_ms() == 5000) state.car.gas_pedal = 0;
        if (esp_timer_get_time_ms() == 9000) state.car.gas_pedal = 0.3;
        if (esp_timer_get_time_ms() == 11200) state.car.gas_pedal *= 0.75;
        if (esp_timer_get_time_ms() == 11500) state.car.gas_pedal *= 0.5;
        if (esp_timer_get_time_ms() == 11700) state.car.gas_pedal *= 0.5;
        if (esp_timer_get_time_ms() == 12000) state.car.gas_pedal = 0;
        if (esp_timer_get_time_ms() == 12000) state.speed_control.pedal_control.target_value = 0.99;
        if (esp_timer_get_time_ms() == 15000) control_buttons_handle(&state, BUTTON_VOLUME_UP);
        if (esp_timer_get_time_ms() == 15000) control_buttons_handle(&state, BUTTON_VOLUME_UP);
        if (esp_timer_get_time_ms() == 15000) control_buttons_handle(&state, BUTTON_VOLUME_UP);
        if (esp_timer_get_time_ms() == 15000) control_buttons_handle(&state, BUTTON_VOLUME_UP);
        if (esp_timer_get_time_ms() == 16000) control_buttons_handle(&state, BUTTON_VOLUME_UP);
        if (esp_timer_get_time_ms() == 16100) control_buttons_handle(&state, BUTTON_VOLUME_UP_LONG_PRESS);
        if (esp_timer_get_time_ms() == 18000) state.speed_control.pedal_control.target_value = 0.01;
        if (esp_timer_get_time_ms() == 18000) control_buttons_handle(&state, BUTTON_VOLUME_DOWN);
        if (esp_timer_get_time_ms() == 18300) control_buttons_handle(&state, BUTTON_VOLUME_DOWN);
        if (esp_timer_get_time_ms() == 18300) control_buttons_handle(&state, BUTTON_VOLUME_DOWN);
        if (esp_timer_get_time_ms() == 18300) control_buttons_handle(&state, BUTTON_VOLUME_DOWN);
        if (esp_timer_get_time_ms() == 18900) control_buttons_handle(&state, BUTTON_VOLUME_DOWN_LONG_PRESS);
        if (esp_timer_get_time_ms() == 19000) printf("%lf\n", state.speed_control.pedal_control.target_value);
        if (esp_timer_get_time_ms() == 25000) control_buttons_handle(&state, BUTTON_SOURCE);

        simulate_car_step(&state, STEP);

        control_cruise_control(&state);
        control_process_car(&state);

        graph_add(&speed_graph, state.car.speed);
        graph_add(&gas_pedal_graph, _get_gas_penal_enabled() ? state.speed_control.virtual_gas_pedal: state.car.gas_pedal);
        graph_add(&acceleration_graph, state.car.acceleration);

        step_time(STEP);

        max_speed = max(max_speed, state.car.speed);
        min_speed = min(min_speed, state.car.speed);
    }

    graph_render(&speed_graph, "../../../test_output/speed_graph.bmp");
    graph_render(&gas_pedal_graph, "../../../test_output/gas_pedal_graph.bmp");
    graph_render(&acceleration_graph, "../../../test_output/acceleration_graph.bmp");
    printf("Max speed: %lf\n", max_speed);
    printf("Min speed: %lf\n", min_speed);
}
