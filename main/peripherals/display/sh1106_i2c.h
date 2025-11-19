//
// Created by Samuel-Anton Jansen on 2025/11/07.
//

#ifndef CARCOMPUTER_SH1106_I2C_H
#define CARCOMPUTER_SH1106_I2C_H

#include <stdint.h>

#include "sh1106.h"

int sh1106_send_byte(SH1106Config *config, uint8_t data);
void sh1106_display(SH1106Config *config);
int sh1106_i2c_init(SH1106Config *config);

#endif //CARCOMPUTER_SH1106_I2C_H
