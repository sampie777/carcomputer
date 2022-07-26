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
    uint8_t buffer[DISPLAY_HEIGHT][DISPLAY_WIDTH];
} SH1106Config;

typedef enum {
    FONT_SMALL = 1,
//    FONT_MEDIUM = 2,
//    FONT_LARGE = 3,
} FontSize;

void sh1106_init(SH1106Config *config);
void sh1106_display(SH1106Config *config);
void sh1106_clear(SH1106Config *config);
void sh1106_zigzag(SH1106Config *config);
void sh1106_draw_char(SH1106Config *config, int x, int y, FontSize size, char c);
void sh1106_draw_string(SH1106Config *config, int x, int y, FontSize size, char *c, size_t length);

#endif //APP_TEMPLATE_SH1106_H
