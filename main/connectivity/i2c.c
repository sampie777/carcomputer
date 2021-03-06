//
// Created by samuel on 22-7-22.
//

#include <driver/i2c.h>
#include <hal/i2c_ll.h>
#include "i2c.h"
#include "../config.h"

void i2c_init() {
    i2c_config_t i2c_config = {
            .mode = I2C_MODE_MASTER,
            .sda_io_num = MAIN_I2C_SDA_PIN,
            .scl_io_num = MAIN_I2C_SCL_PIN,
            .sda_pullup_en = GPIO_PULLUP_ENABLE,
            .scl_pullup_en = GPIO_PULLUP_ENABLE,
            .master.clk_speed = I2C_FREQUENCY_HZ,
            .clk_flags = 0
    };
    ESP_ERROR_CHECK(i2c_param_config(MAIN_I2C_PORT, &i2c_config));
    ESP_ERROR_CHECK(i2c_driver_install(MAIN_I2C_PORT, i2c_config.mode, 0, 0, 0));

    i2c_config_t i2c_config_display = {
            .mode = I2C_MODE_MASTER,
            .sda_io_num = DISPLAY_I2C_SDA_PIN,
            .scl_io_num = DISPLAY_I2C_SCL_PIN,
            .sda_pullup_en = GPIO_PULLUP_ENABLE,
            .scl_pullup_en = GPIO_PULLUP_ENABLE,
            .master.clk_speed = I2C_FREQUENCY_HZ,
            .clk_flags = 0
    };
    ESP_ERROR_CHECK(i2c_param_config(DISPLAY_I2C_PORT, &i2c_config_display));
    ESP_ERROR_CHECK(i2c_driver_install(DISPLAY_I2C_PORT, i2c_config_display.mode, 0, 0, 0));
    printf("[i2c] Init done\n");
}