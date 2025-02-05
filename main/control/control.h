//
// Created by samuel on 19-7-22.
//

#ifndef APP_TEMPLATE_CONTROL_H
#define APP_TEMPLATE_CONTROL_H

#include "../state.h"

#define ERROR_CODES_0_WAIT5SEC_MS (5000)
#define ERROR_CODES_1_WAIT3SEC_MS (3000)
#define ERROR_CODES_2_DEPRESS_PEDAL_COUNT (5)
#define ERROR_CODES_2_DEPRESS_PEDAL_INTERVAL (400)
#define ERROR_CODES_3_WAIT7SEC_MS (7000)
#define ERROR_CODES_4_DEPRESS_PEDAL_FULLY_TIME (10000)

void control_init(State *state);

void control_read_can_bus(State *state);

void control_read_analog_sensors(State *state);

void control_read_user_input(State *state);

void control_door_lock(State *state);

void control_engine_shutoff(State *state);

void control_cruise_control(State *state);

void control_process_car(State *state);

void control_led_indicator_step(State *state);

void control_mpu_power(State *state);

void control_crash_detection(State *state);

void control_read_error_codes(State* state);

#endif //APP_TEMPLATE_CONTROL_H
