//
// Created by samuel on 31-7-22.
//

#ifndef APP_TEMPLATE_MPU9250_H
#define APP_TEMPLATE_MPU9250_H

#include "../state.h"

#define GYRO_X_OFFSET 0.56
#define GYRO_Y_OFFSET 0.33
#define GYRO_Z_OFFSET -0.16

void mpu9250_read(State *state);

void mpu9250_init();

#endif //APP_TEMPLATE_MPU9250_H
