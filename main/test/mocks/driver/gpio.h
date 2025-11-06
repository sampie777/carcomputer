//
// Created by Samuel-Anton Jansen on 2025/11/06.
//

#ifndef CARCOMPUTER_GPIO_H
#define CARCOMPUTER_GPIO_H
#include <stdint.h>
#include <esp_err.h>
#include <soc/gpio_num.h>
#include <hal/gpio_types.h>

esp_err_t gpio_set_level(gpio_num_t gpio_num, uint32_t level) {return 0;}
esp_err_t gpio_set_direction(gpio_num_t gpio_num, gpio_mode_t mode);

#endif //CARCOMPUTER_GPIO_H