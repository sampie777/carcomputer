//
// Created by samuel on 25-7-22.
//

#ifndef APP_TEMPLATE_SH1106_H
#define APP_TEMPLATE_SH1106_H

#include "../config.h"

typedef struct {
    int address;
    int8_t flip;
    uint8_t buffer[DISPLAY_HEIGHT][DISPLAY_WIDTH];
} SH1106Config;

void sh1106_init(SH1106Config *config);
void sh1106_display(SH1106Config *config);
void sh1106_clear(SH1106Config *config);
void sh1106_zigzag(SH1106Config *config);

#endif //APP_TEMPLATE_SH1106_H
