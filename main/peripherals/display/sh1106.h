//
// Created by samuel on 25-7-22.
//

#ifndef APP_TEMPLATE_SH1106_H
#define APP_TEMPLATE_SH1106_H

#include "../../config.h"

typedef struct {
    int address;
    int8_t flip;
    int height;
    int width;
    uint8_t *buffer[DISPLAY_HEIGHT];
} SH1106Config;

typedef enum {
    FONT_SMALL = 1,
//    FONT_MEDIUM = 2,
//    FONT_LARGE = 3,
} FontSize;

typedef enum {
    FONT_WHITE,
    FONT_BLACK,
} FontColor;

void sh1106_init(SH1106Config *config);
void sh1106_display(SH1106Config *config);
void sh1106_clear(SH1106Config *config);
void sh1106_draw_char(SH1106Config *config, int x, int y, FontSize size, char c, FontColor color);
int sh1106_draw_string(SH1106Config *config, int x, int y, FontSize size, FontColor color, size_t length, char *c);
void sh1106_draw_horizontal_line(SH1106Config *config, int x, int y, int length);
void sh1106_draw_vertical_line(SH1106Config *config, int x, int y, int length);
void sh1106_draw_rectangle(SH1106Config *config, int x, int y, int width, int height);
void sh1106_draw_filled_rectangle(SH1106Config *config, int x, int y, int width, int height);
void sh1106_draw_icon(SH1106Config *config, int x, int y, const unsigned char *icon, size_t icon_size, int icon_width, FontColor color);

#endif //APP_TEMPLATE_SH1106_H
