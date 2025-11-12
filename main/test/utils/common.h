//
// Created by Samuel-Anton Jansen on 2025/11/12.
//

#ifndef CARCOMPUTER_COMMON_H
#define CARCOMPUTER_COMMON_H

#include <stdint.h>
#include "../../state.h"

void step_time(uint64_t delta);
void simulate_car_step(State* state, int32_t delta);

#endif //CARCOMPUTER_COMMON_H
