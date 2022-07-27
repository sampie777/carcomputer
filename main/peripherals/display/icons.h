//
// Created by samuel on 27-7-22.
//

#ifndef APP_TEMPLATE_ICONS_H
#define APP_TEMPLATE_ICONS_H

static const int icon_bluetooth_width = 6;
static const unsigned char icon_bluetooth[] = {
        0x88, // 10001000
        0x50, // 01010000
        0xFF, // 11111111
        0x22, // 00100010
        0x54, // 01010100
        0x88, // 10001000

        0x00, // 00000000
        0x00, // 00000000
        0x07, // 00000111
        0x02, // 00000010
        0x01, // 00000001
        0x00, // 00000000
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

#endif //APP_TEMPLATE_ICONS_H
