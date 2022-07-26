//
// Created by samuel on 25-7-22.
//

#ifndef APP_TEMPLATE_SH1106_H
#define APP_TEMPLATE_SH1106_H

typedef struct {
    int address;
    int flip;
} SH1106Config;

void sh1106_init(SH1106Config *config);
void sh1106_display(SH1106Config *config);
void sh1106_clear(SH1106Config *config);

#endif //APP_TEMPLATE_SH1106_H
