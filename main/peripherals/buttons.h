//
// Created by samuel on 28-7-22.
//

#ifndef APP_TEMPLATE_BUTTONS_H
#define APP_TEMPLATE_BUTTONS_H

typedef enum {
    BUTTON_NONE,
    BUTTON_VOLUME_UP,      // 1967
    BUTTON_VOLUME_DOWN,    // 1968
    BUTTON_INFO,
    BUTTON_SOURCE,         // 4095
    BUTTON_UP,             // 3217
    BUTTON_DOWN,           // 3216
    BUTTON_VOLUME_UP_LONG_PRESS,
    BUTTON_VOLUME_DOWN_LONG_PRESS,
    BUTTON_INFO_LONG_PRESS,
    BUTTON_SOURCE_LONG_PRESS,
    BUTTON_UP_LONG_PRESS,
    BUTTON_DOWN_LONG_PRESS,
} Button;

Button buttons_get_pressed();

void buttons_init();

#endif //APP_TEMPLATE_BUTTONS_H
