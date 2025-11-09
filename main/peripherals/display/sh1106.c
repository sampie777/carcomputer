//
// Created by samuel on 25-7-22.
//

#include "sh1106.h"
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "font.h"
#include "sh1106_i2c.h"
#include "../../utils.h"


uint8_t **scale_data(const uint8_t *data, int data_length, int scale, int *scaled_data_cols, int *scaled_data_rows) {
    *scaled_data_rows = scale;
    *scaled_data_cols = scale * data_length;

    uint8_t **scaled_data = malloc(sizeof(uint8_t *) * (*scaled_data_rows));
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
    for (int i = 0; i < config->height >> 5; i++) {
        memset(config->buffer[i], 0, config->width);
    }
}

void sh1106_draw_pixel(SH1106Config *config, int x, int y, FontColor color) {
    if (x < 0 || x >= config->width) return;
    if (y < 0 || y >= config->height) return;

    int top_row = y >> 5;
    int char_y = y % 32;

    if (color == FONT_BLACK) {
        config->buffer[top_row][x] &= ~(1 << char_y);
    } else {
        config->buffer[top_row][x] |= 1 << char_y;
    }
}

FontColor sh1106_read_pixel(SH1106Config *config, int x, int y) {
    if (x < 0 || x >= config->width) return FONT_BLACK;
    if (y < 0 || y >= config->height) return FONT_BLACK;

    int top_row = y >> 5;
    int char_y = y % 32;

    int mask = 1 << char_y;
    return (config->buffer[top_row][x] & mask) ? FONT_WHITE : FONT_BLACK;
}

void sh1106_draw_byte(SH1106Config *config, int x, int y, unsigned char data, FontColor color) {
    if (x < 0 || x >= config->width) return;
    if (y < -7 || y >= config->height) return;

    int top_row = y >> 5;
    int bottom_row = top_row + 1;
    int char_y = y >= 0 ? y % 32 : 32 + (y % 32);
    int char_y_overflow = 32 - char_y;

    if (color == FONT_BLACK) {
        config->buffer[top_row][x] &= ~(data << char_y);
        if (bottom_row < (config->height >> 5) && char_y_overflow <= 8)
            config->buffer[bottom_row][x] &= ~(data >> char_y_overflow);
    } else {
        config->buffer[top_row][x] |= data << char_y;
        if (bottom_row < (config->height >> 5) && char_y_overflow <= 8)
            config->buffer[bottom_row][x] |= data >> char_y_overflow;
    }
}

void sh1106_draw_char(SH1106Config *config, int x, int y, FontSize size, FontColor color, uint8_t c) {
    int font_char_index = c * font_width;

    if (size == FONT_SMALL) {
        for (int col = 0; col < font_width; col++) {
            sh1106_draw_byte(config, x + col, y, font[font_char_index + col], color);
        }
    } else {
        int scaled_data_cols, scaled_data_rows;
        uint8_t **scaled_data = scale_data(&font[font_char_index], font_width, size, &scaled_data_cols,
                                           &scaled_data_rows);

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
 * @param text_spacing The minimum extra spacing between the letters (default = 0 as this will result in a 1 pixel gap between each letter)
 * @return the total horizontal pixel length used to draw the string
 */
int sh1106_draw_string_with_spacing(SH1106Config *config, int x, int y, FontSize size, FontColor color,
                                    const uint8_t *c,
                                    int text_spacing) {
    int letter_spacing = 0;
    for (int i = 0; i < strlen(c); i++) {
        // If current char starts with empty space, move it a bit to the left
        if (font[c[i] * font_width] == 0x00) {
            letter_spacing--;
        }

        sh1106_draw_char(config, x + i * font_width * (int) size + letter_spacing * (int) size, y, size, color, c[i]);

        // If current char does not end with empty space, move the next char a bit to the right
        if (font[c[i] * font_width + font_width - 1] != 0x00) {
            letter_spacing++;
        }

        if (text_spacing != 0 && i < strlen(c) - 1) {
            letter_spacing += text_spacing;
        }
    }

    return (int) strlen(c) * font_width * (int) size + letter_spacing * (int) size;
}

/**
 *
 * @param config
 * @param x
 * @param y
 * @param size
 * @param c
 * @return the total horizontal pixel length used to draw the string
 */
int sh1106_draw_string(SH1106Config *config, int x, int y, FontSize size, FontColor color, const char *c) {
    return sh1106_draw_string_with_spacing(config, x, y, size, color, c, 0);
}

int sh1106_draw_string_centered_x(SH1106Config *config, int y, FontSize size, FontColor color,
                                  const char *c) {
    int length = (int) strlen(c);
    int x = (int) round((config->width - (length + 1) * 5 * size) / 2.0);
    return sh1106_draw_string(config, x, y, size, color, c);
}

void sh1106_draw_horizontal_line(SH1106Config *config, int x, int y, int length) {
    if (length <= 0 || y < 0 || y >= config->height) {
        return;
    }

    int row = y >> 5;
    int char_y = y % 32;

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

    int top_row = y >> 5;
    int bottom_row = (y + length) >> 5;

    for (int row = top_row; row <= bottom_row; row++) {
        if (row < 0 || row >= (config->height >> 5)) {
            continue;
        }

        uint32_t mask = 0xffffffff;

        if (row == top_row) {
            mask &= 0xffffffff << y % 32;
        }
        if (row == bottom_row) {
            mask &= 0xffffffff >> (32 - (y + length) % 32);
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

void sh1106_draw_circle(SH1106Config *config, int x, int y, int radius, FontColor color) {
    if (radius <= 0) return;

    int x0 = x;
    int y0 = y;
    int f = 1 - radius;
    int ddF_x = 1;
    int ddF_y = -2 * radius;
    int x1 = 0;
    int y1 = radius;

    sh1106_draw_pixel(config, x0, y0 + radius, color);
    sh1106_draw_pixel(config, x0, y0 - radius, color);
    sh1106_draw_pixel(config, x0 + radius, y0, color);
    sh1106_draw_pixel(config, x0 - radius, y0, color);

    while (x1 < y1) {
        if (f >= 0) {
            y1--;
            ddF_y += 2;
            f += ddF_y;
        }
        x1++;
        ddF_x += 2;
        f += ddF_x;

        sh1106_draw_pixel(config, x0 + x1, y0 + y1, color);
        sh1106_draw_pixel(config, x0 - x1, y0 + y1, color);
        sh1106_draw_pixel(config, x0 + x1, y0 - y1, color);
        sh1106_draw_pixel(config, x0 - x1, y0 - y1, color);
        sh1106_draw_pixel(config, x0 + y1, y0 + x1, color);
        sh1106_draw_pixel(config, x0 - y1, y0 + x1, color);
        sh1106_draw_pixel(config, x0 + y1, y0 - x1, color);
        sh1106_draw_pixel(config, x0 - y1, y0 - x1, color);
    }
}

void sh1106_draw_icon(SH1106Config *config, int x, int y, const unsigned char *icon, size_t icon_size, int icon_width,
                      FontColor color) {
    for (int i = 0; i < icon_size; i++) {
        int col = x + (i % icon_width);
        int row = y + 8 * (i / icon_width);
        sh1106_draw_byte(config, col, row, icon[i], color);
    }
}


int sh1106_init(SH1106Config *config) {
    printf("[sh1106] Initializing...\n");

    config->buffer = malloc((config->height >> 5) * sizeof(uint32_t *));
    for (int i = 0; i < config->height >> 5; i++) {
        config->buffer[i] = malloc(config->width * sizeof(uint32_t));
    }

    int result = sh1106_i2c_init(config);
    printf("[sh1106] Init done\n");
    return result;
}
