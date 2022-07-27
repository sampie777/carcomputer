//
// Created by samuel on 27-7-22.
//

#ifndef APP_TEMPLATE_ICONS_H
#define APP_TEMPLATE_ICONS_H

static const int icon_bluetooth_width = 5;
static const unsigned char icon_bluetooth[] = {
        0x14, // 00010100
        0x08, // 00001000
        0x7F, // 01111111
        0x2A, // 00101010
        0x14, // 00010100
};

static const int icon_wifi_width = 9;
static const unsigned char icon_wifi[] = {
        0x08, // 00001000
        0x04, // 00000100
        0x12, // 00010010
        0x0A, // 00001010
        0x2A, // 00101010
        0x0A, // 00001010
        0x12, // 00010010
        0x04, // 00000100
        0x08, // 00001000
};

static const int icon_car_width = 6;
static const unsigned char icon_car[] = {
        0x1C, // 00011100
        0x37, // 00110111
        0x1F, // 00011111
        0x1F, // 00011111
        0x37, // 00110111
        0x1C, // 00011100
};

#endif //APP_TEMPLATE_ICONS_H
