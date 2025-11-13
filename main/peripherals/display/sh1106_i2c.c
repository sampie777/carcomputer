//
// Created by Samuel-Anton Jansen on 2025/11/07.
//

#include "sh1106_i2c.h"

#include <stdbool.h>
#include <driver/i2c.h>
#include "../../return_codes.h"
#include "../../utils.h"

#define CONTROL_BYTE_CONFIG_SINGLE_DATA 0x80
#define CONTROL_BYTE_CONFIG_MULTI_DATA 0x00
#define CONTROL_BYTE_RAM_SINGLE_DATA 0xC0
#define CONTROL_BYTE_RAM_MULTI_DATA 0x40

#define SH1106_CONFIG_SET_COLUMN_LOW 0x00
#define SH1106_CONFIG_SET_COLUMN_HIGH 0x10
#define SH1106_CONFIG_SET_MEMORY_MODE 0x20
#define SH1106_CONFIG_SET_START_LINE 0x40
#define SH1106_CONFIG_SET_CONTRAST 0x81
#define SH1106_CONFIG_SET_CHARGE_PUMP 0x8D
#define SH1106_CONFIG_SET_SEGMENT_REMAP 0xA0
#define SH1106_CONFIG_SET_DISPLAY_RESUME 0xA4
#define SH1106_CONFIG_SET_DISPLAY_FORCE_ON 0xA5
#define SH1106_CONFIG_SET_DISPLAY_NON_INVERTED 0xA6
#define SH1106_CONFIG_SET_DISPLAY_INVERTED 0xA7
#define SH1106_CONFIG_SET_MULTIPLEX 0xA8
#define SH1106_CONFIG_SET_DISPLAY_OFF 0xAE
#define SH1106_CONFIG_SET_DISPLAY_ON 0xAF
#define SH1106_CONFIG_SET_PAGE 0xB0
#define SH1106_CONFIG_SET_COMMON_OUTPUT_SCAN_DIRECTION 0xC0
#define SH1106_CONFIG_SET_DISPLAY_OFFSET 0xD3
#define SH1106_CONFIG_SET_CLOCK_DIVIDER 0xD5
#define SH1106_CONFIG_SET_PRE_CHARGE 0xD9
#define SH1106_CONFIG_SET_COMMON_PADS 0xDA
#define SH1106_CONFIG_SET_VCOM_DESELECT_LEVEL 0xDB

#define SH1106_COL_OFFSET 0x02


int sh1106_send_byte(SH1106Config *config, uint8_t data) {
    static bool has_shown_error = false;

    i2c_cmd_handle_t command = i2c_cmd_link_create();
    i2c_master_start(command);
    i2c_master_write_byte(command, (config->address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(command, CONTROL_BYTE_CONFIG_MULTI_DATA, true);
    i2c_master_write_byte(command, data, true);
    i2c_master_stop(command);

    esp_err_t result = i2c_master_cmd_begin(DISPLAY_I2C_PORT, command, I2C_TIMEOUT_MS / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(command);
    if (result != ESP_OK) {
        config->transmission_failures++;
        if (!has_shown_error) {
            printf("[sh1106] I2C byte transmission failed: 0x%03x %s\n", result, esp_err_to_name(result));
            has_shown_error = true;
        }
        if (result == ESP_ERR_TIMEOUT) return RESULT_DISCONNECTED;
        return RESULT_FAILED;
    }

    has_shown_error = false;
    return RESULT_OK;
}

void sh1106_display(SH1106Config *config) {
    if (sh1106_send_byte(config, SH1106_CONFIG_SET_START_LINE | 0x00) != RESULT_OK) return;

    for (int row = 0; row < config->height >> 5; row++) {
        for (int subrow = 0; subrow < 4; subrow++) {

            uint8_t subrow_data[config->width];
            for (int i = 0; i < config->width; i++) {
                subrow_data[i] = (uint8_t) (config->buffer[row][i] >> subrow * 8);
            }

            i2c_cmd_handle_t command = i2c_cmd_link_create();
            i2c_master_start(command);
            i2c_master_write_byte(command, (config->address << 1) | I2C_MASTER_WRITE, true);
            i2c_master_write_byte(command, CONTROL_BYTE_CONFIG_SINGLE_DATA, true);
            i2c_master_write_byte(command, SH1106_CONFIG_SET_PAGE | (row * 4 + subrow), true);
            i2c_master_write_byte(command, CONTROL_BYTE_CONFIG_SINGLE_DATA, true);
            i2c_master_write_byte(command, SH1106_CONFIG_SET_COLUMN_LOW | SH1106_COL_OFFSET, true);
            i2c_master_write_byte(command, CONTROL_BYTE_CONFIG_SINGLE_DATA, true);
            i2c_master_write_byte(command, SH1106_CONFIG_SET_COLUMN_HIGH | (SH1106_COL_OFFSET >> 4), true);

            i2c_master_write_byte(command, CONTROL_BYTE_RAM_MULTI_DATA, true);

            // For some reason the next if-statement cannot be put in a C if-statement.
            // The display will then show scrambled pixels if DISPLAY_UPSIDE_DOWN == true.
#if DISPLAY_UPSIDE_DOWN
            uint8_t data[config->width];
            invert_array(subrow_data, data, config->width);
            i2c_master_write(command, data, config->width, true);
#else
            i2c_master_write(command, config->buffer[row], config->width, true);
#endif

            i2c_master_stop(command);
            if (i2c_master_cmd_begin(DISPLAY_I2C_PORT, command, I2C_TIMEOUT_MS / portTICK_PERIOD_MS) != ESP_OK) {
                printf("[sh1106] I2C graphics transmission failed\n");
                config->transmission_failures++;
            }
            i2c_cmd_link_delete(command);
        }
    }
}

int sh1106_i2c_init(SH1106Config *config) {
    i2c_cmd_handle_t command = i2c_cmd_link_create();
    i2c_master_start(command);
    i2c_master_write_byte(command, (config->address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(command, CONTROL_BYTE_CONFIG_MULTI_DATA, true);

    // Next lines come from https://github.com/wonho-maker/Adafruit_SH1106/blob/master/Adafruit_SH1106.cpp#L261
    i2c_master_write_byte(command, SH1106_CONFIG_SET_DISPLAY_OFF, true);
    i2c_master_write_byte(command, SH1106_CONFIG_SET_CLOCK_DIVIDER, true);
    i2c_master_write_byte(command, 0x80, true);
    i2c_master_write_byte(command, SH1106_CONFIG_SET_MULTIPLEX, true);
    i2c_master_write_byte(command, 0x3F, true);
    i2c_master_write_byte(command, SH1106_CONFIG_SET_DISPLAY_OFFSET, true);
    i2c_master_write_byte(command, 0x00, true);
    i2c_master_write_byte(command, SH1106_CONFIG_SET_START_LINE | 0x00, true);
    i2c_master_write_byte(command, SH1106_CONFIG_SET_CHARGE_PUMP, true);
    i2c_master_write_byte(command, 0x14, true);
    i2c_master_write_byte(command, SH1106_CONFIG_SET_MEMORY_MODE, true);
    i2c_master_write_byte(command, 0x00, true);
    i2c_master_write_byte(command, SH1106_CONFIG_SET_SEGMENT_REMAP | 0x01, true);
    i2c_master_write_byte(
        command, SH1106_CONFIG_SET_COMMON_OUTPUT_SCAN_DIRECTION | (config->mirror_vertical ? 0x00 : 0x08), true);
    i2c_master_write_byte(command, SH1106_CONFIG_SET_COMMON_PADS, true);
    i2c_master_write_byte(command, 0x12, true);

    i2c_master_write_byte(command, SH1106_CONFIG_SET_CONTRAST, true);
    i2c_master_write_byte(command, 0xCF, true);
    i2c_master_write_byte(command, SH1106_CONFIG_SET_PRE_CHARGE, true);
    i2c_master_write_byte(command, 0xF1, true);
    i2c_master_write_byte(command, SH1106_CONFIG_SET_VCOM_DESELECT_LEVEL, true);
    i2c_master_write_byte(command, 0x40, true);
    i2c_master_write_byte(command, SH1106_CONFIG_SET_DISPLAY_NON_INVERTED, true);
    i2c_master_write_byte(command, SH1106_CONFIG_SET_DISPLAY_RESUME, true);

    i2c_master_stop(command);
    esp_err_t result = i2c_master_cmd_begin(DISPLAY_I2C_PORT, command, I2C_TIMEOUT_MS / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(command);
    if (result != ESP_OK) {
        printf("[sh1106] I2C init transmission failed: 0x%03x %s\n", result, esp_err_to_name(result));
        if (result == ESP_ERR_TIMEOUT) return RESULT_DISCONNECTED;
        return RESULT_FAILED;
    }

    sh1106_clear(config);
    sh1106_display(config);
    sh1106_send_byte(config, SH1106_CONFIG_SET_DISPLAY_ON);

    return RESULT_OK;
}
