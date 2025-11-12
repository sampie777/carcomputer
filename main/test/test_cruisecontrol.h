//
// Created by Samuel-Anton Jansen on 2025/11/07.
//

#ifndef CARCOMPUTER_TEST_CRUISECONTROL_H
#define CARCOMPUTER_TEST_CRUISECONTROL_H
#include <stdint.h>
#include "../state.h"

void simulate_car_step(State* state, int32_t delta);
void test_cruise_control(void);
void test_hourly_eta();

#endif //CARCOMPUTER_TEST_CRUISECONTROL_H