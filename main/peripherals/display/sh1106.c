//
// Created by samuel on 25-7-22.
//

#include <stdio.h>
#include <driver/i2c.h>
#include <string.h>
#include "sh1106.h"
#include "font.h"

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


uint8_t **scale_data(const uint8_t *data, int data_length, int scale, int *scaled_data_cols, int *scaled_data_rows) {
    *scaled_data_rows = scale;
    *scaled_data_cols = scale * data_length;

    uint8_t **scaled_data = malloc(sizeof(uint8_t *) * *scaled_data_rows);
    for (int i = 0; i < *scaled_data_rows; i++) {
        scaled_data[i] = malloc(sizeof(uint8_t) * (*scaled_data_cols));
        memset(scaled_data[i], 0, *scaled_data_cols);
    }

    int scaled_x = 0;
    for (int x = 0; x < data_length; x++) {
        for (int y = 0; y < 8; y++) {
            int pixel = (data[x] >> y) & 1;
            int scaled_y = y * scale;

            // Fill in the rectangle as the enlarged pixel
            for (int px = 0; px < scale; px++) {
                for (int py = 0; py < scale; py++) {
                    int scaled_pixel_y = (scaled_y + py) % 8;
                    scaled_data[(scaled_y + py) / 8][scaled_x + px] |= pixel << scaled_pixel_y;
                }
            }
        }

        scaled_x += scale;
    }
    return scaled_data;
}

void sh1106_clear(SH1106Config *config) {
    for (int i = 0; i < config->height; i++) {
        memset(config->buffer[i], 0, config->width);
    }
}

void sh1106_draw_byte(SH1106Config *config, int x, int y, unsigned char data, FontColor color) {
    if (x < 0 || x >= config->width) return;
    if (y < -7 || y >= config->height) return;

    int top_row = y >> 3;
    int bottom_row = top_row + 1;
    int char_y = y >= 0 ? y % 8 : 8 + (y % 8);

    if (color == FONT_BLACK) {
        config->buffer[top_row][x] &= ~(data << char_y);
        config->buffer[bottom_row][x] &= ~(data >> (8 - char_y));
    } else {
        config->buffer[top_row][x] |= data << char_y;
        config->buffer[bottom_row][x] |= data >> (8 - char_y);
    }
}

void sh1106_draw_char(SH1106Config *config, int x, int y, FontSize size, FontColor color, char c) {
    int font_char_index = c * font_width;

    if (size == FONT_SMALL) {
        for (int col = 0; col < font_width; col++) {
            sh1106_draw_byte(config, x + col, y, font[font_char_index + col], color);
        }
    } else {
        int scaled_data_cols, scaled_data_rows;
        uint8_t **scaled_data = scale_data(&font[font_char_index], font_width, size, &scaled_data_cols, &scaled_data_rows);

        for (int row = 0; row < scaled_data_rows; row++) {
            for (int col = 0; col < scaled_data_cols; col++) {
                sh1106_draw_byte(config, x + col, y + row * 8, scaled_data[row][col], color);
            }
        }

        for (int i = 0; i < scaled_data_rows; i++) {
            free(scaled_data[i]);
        }
        free(scaled_data);
    }
}

/**
 *
 * @param config
 * @param x
 * @param y
 * @param size
 * @param c
 * @param length
 * @return the total horizontal pixel length used to draw the string
 */
int sh1106_draw_string(SH1106Config *config, int x, int y, FontSize size, FontColor color, const char *c) {
    int letter_spacing = 0;
    for (int i = 0; i < strlen(c); i++) {
        // If current char starts with empty space, move it a bit to the left
        if (font[c[i] * font_width] == 0x00) {
            letter_spacing--;
        }

        sh1106_draw_char(config, x + i * font_width * (int) size + letter_spacing * (int) size, y, size, color, c[i]);

        // If current char ends with empty space, move the next char a bit to the right
        if (font[c[i] * font_width + font_width - 1] != 0x00) {
            letter_spacing++;
        }
    }

    return (int) strlen(c) * font_width * (int) size + letter_spacing * (int) size - 1;
}

void sh1106_draw_horizontal_line(SH1106Config *config, int x, int y, int length) {
    if (length <= 0 || y < 0 || y >= config->height) {
        return;
    }

    int row = y >> 3;
    int char_y = y % 8;

    for (int col = 0; col < length; col++) {
        // Allow overflow horizontal edges
        if (x + col < 0 || x + col >= config->width) {
            continue;
        }

        config->buffer[row][x + col] |= 0x01 << char_y;
    }
}

void sh1106_draw_vertical_line(SH1106Config *config, int x, int y, int length) {
    if (length <= 0 || x < 0 || x >= config->width) {
        return;
    }

    int top_row = y >> 3;
    int bottom_row = (y + length) >> 3;

    for (int row = top_row; row <= bottom_row; row++) {
        if (row < 0 || row >= config->height / 8) {
            continue;
        }

        int mask = 0xff;

        if (row == top_row) {
            mask &= 0xff << y % 8;
        }
        if (row == bottom_row) {
            mask &= 0xff >> (8 - (y + length) % 8);
        }
        config->buffer[row][x] |= mask;
    }
}

void sh1106_draw_rectangle(SH1106Config *config, int x, int y, int width, int height) {
    if (width <= 0 || height <= 0) return;
    sh1106_draw_vertical_line(config, x, y, height);
    sh1106_draw_vertical_line(config, x + width - 1, y, height);
    sh1106_draw_horizontal_line(config, x, y, width);
    sh1106_draw_horizontal_line(config, x, y + height - 1, width);
}

void sh1106_draw_filled_rectangle(SH1106Config *config, int x, int y, int width, int height) {
    if (width <= 0 || height <= 0) return;
    for (int i = 0; i < width; i++) {
        sh1106_draw_vertical_line(config, x + i, y, height);
    }
}

void sh1106_draw_icon(SH1106Config *config, int x, int y, const unsigned char *icon, size_t icon_size, int icon_width, FontColor color) {
    for (int i = 0; i < icon_size; i++) {
        int col = x + (i % icon_width);
        int row = y + 8 * (i / icon_width);
        sh1106_draw_byte(config, col, row, icon[i], color);
    }
}

//
// I2C INTERACTION
//

void sh1106_send_byte(SH1106Config *config, uint8_t data) {
    i2c_cmd_handle_t command = i2c_cmd_link_create();
    i2c_master_start(command);
    i2c_master_write_byte(command, (config->address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(command, CONTROL_BYTE_CONFIG_MULTI_DATA, true);
    i2c_master_write_byte(command, data, true);
    i2c_master_stop(command);

    if (i2c_master_cmd_begin(DISPLAY_I2C_PORT, command, I2C_TIMEOUT_MS / portTICK_PERIOD_MS) != ESP_OK) {
        printf("[sh1106] I2C byte transmission failed\n");
    }
    i2c_cmd_link_delete(command);
}

void sh1106_display(SH1106Config *config) {
    sh1106_send_byte(config, SH1106_CONFIG_SET_START_LINE | 0x00);

    for (int row = 0; row < (config->height >> 3); row++) {
        i2c_cmd_handle_t command = i2c_cmd_link_create();
        i2c_master_start(command);
        i2c_master_write_byte(command, (config->address << 1) | I2C_MASTER_WRITE, true);
        i2c_master_write_byte(command, CONTROL_BYTE_CONFIG_SINGLE_DATA, true);
        i2c_master_write_byte(command, SH1106_CONFIG_SET_PAGE | row, true);
        i2c_master_write_byte(command, CONTROL_BYTE_CONFIG_SINGLE_DATA, true);
        i2c_master_write_byte(command, SH1106_CONFIG_SET_COLUMN_LOW | SH1106_COL_OFFSET, true);
        i2c_master_write_byte(command, CONTROL_BYTE_CONFIG_SINGLE_DATA, true);
        i2c_master_write_byte(command, SH1106_CONFIG_SET_COLUMN_HIGH | (SH1106_COL_OFFSET >> 4), true);

        i2c_master_write_byte(command, CONTROL_BYTE_RAM_MULTI_DATA, true);
        i2c_master_write(command, config->buffer[row], config->width, true);

        i2c_master_stop(command);
        if (i2c_master_cmd_begin(DISPLAY_I2C_PORT, command, I2C_TIMEOUT_MS / portTICK_PERIOD_MS) != ESP_OK) {
            printf("[sh1106] I2C graphics transmission failed\n");
        }
        i2c_cmd_link_delete(command);
    }
}

void sh1106_init(SH1106Config *config) {
    printf("[sh1106] Initializing...\n");

    config->buffer = malloc(config->height * sizeof(uint8_t *));
    for (int i = 0; i < config->height; i++) {
        config->buffer[i] = malloc(config->width * sizeof(uint8_t));
    }

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
    i2c_master_write_byte(command, SH1106_CONFIG_SET_COMMON_OUTPUT_SCAN_DIRECTION | (config->flip ? 0x00 : 0x08), true);
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
    if (i2c_master_cmd_begin(DISPLAY_I2C_PORT, command, I2C_TIMEOUT_MS / portTICK_PERIOD_MS) != ESP_OK) {
        printf("[sh1106] I2C init transmission failed\n");
    }
    i2c_cmd_link_delete(command);

    sh1106_clear(config);
    sh1106_display(config);
    sh1106_send_byte(config, SH1106_CONFIG_SET_DISPLAY_ON);

    printf("[sh1106] Init done\n");
}