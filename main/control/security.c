//
// Created by samuel on 6-9-22.
//

#include <driver/gpio.h>
#include "security.h"

void security_init() {
    gpio_set_direction(CAR_CLAXON_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(CAR_ENGINE_SHUTOFF_DISABLE_PIN, GPIO_MODE_OUTPUT);
}

void security_step(State *state) {
    gpio_set_level(CAR_ENGINE_SHUTOFF_DISABLE_PIN, 1);
    gpio_set_level(CAR_CLAXON_PIN, 0);

    // if alarm goes off, enable virtual gas pedal and set pedal to 0%
}
