//
// Created by Samuel-Anton Jansen on 2025/11/07.
//

#include "sh1106_i2c.h"
#include "../../../../../return_codes.h"

SH1106Config* _config = NULL;

SH1106Config* getConfig() {
    return _config;
}

int sh1106_send_byte(SH1106Config* config, uint8_t data) {
    return RESULT_OK;
}

void sh1106_display(SH1106Config* config) {}

int sh1106_i2c_init(SH1106Config* config) {
    _config = config;
    return RESULT_OK;
}

