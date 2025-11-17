//
// Created by samuel on 25-7-22.
//

#ifndef APP_TEMPLATE_SH1106_H
#define APP_TEMPLATE_SH1106_H

#include <stdbool.h>
#include <stdlib.h>

typedef struct {
    int address;
    bool mirror_vertical;
    int height;
    int width;
    int transmission_failures;
    uint32_t **buffer;
} SH1106Config;

typedef enum {
    FONT_SMALL = 1,
    FONT_MEDIUM = 2,
    FONT_LARGE = 3,
    FONT_EXTRA_LARGE = 5,
} FontSize;

typedef enum {
    FONT_WHITE,
    FONT_BLACK,
} FontColor;

int sh1106_init(SH1106Config *config);
void sh1106_clear(SH1106Config *config);
void sh1106_draw_pixel(SH1106Config *config, int x, int y, FontColor color);
FontColor sh1106_read_pixel(SH1106Config *config, int x, int y);
void sh1106_draw_char(SH1106Config *config, int x, int y, FontSize size, FontColor color, uint8_t c);
int sh1106_draw_string(SH1106Config *config, int x, int y, FontSize size, FontColor color, const char *c);
int sh1106_draw_string_centered_x(SH1106Config *config, int y, FontSize size, FontColor color, const char *c);
int sh1106_draw_string_with_spacing(SH1106Config *config, int x, int y, FontSize size,
                                    FontColor color, const uint8_t *c, int text_spacing);
void sh1106_draw_horizontal_line(SH1106Config *config, int x, int y, int length);
void sh1106_draw_vertical_line(SH1106Config *config, int x, int y, int length);
void sh1106_draw_rectangle(SH1106Config *config, int x, int y, int width, int height);
void sh1106_draw_filled_rectangle(SH1106Config *config, int x, int y, int width, int height);
void sh1106_draw_circle(SH1106Config *config, int x, int y, int radius, FontColor color);
void sh1106_draw_icon(SH1106Config *config, int x, int y, const unsigned char *icon,
                      size_t icon_size, int icon_width, FontColor color);

#endif //APP_TEMPLATE_SH1106_H
