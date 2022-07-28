//
// Created by samuel on 28-7-22.
//

#ifndef APP_TEMPLATE_BUTTONS_H
#define APP_TEMPLATE_BUTTONS_H

typedef enum {
    NONE,
    VOLUME_UP,      // 510
    VOLUME_DOWN,    // 510
    INFO,
    SOURCE,         // 985
    UP,             // 798
    DOWN,           // 798
    VOLUME_UP_LONG_PRESS,
    VOLUME_DOWN_LONG_PRESS,
    INFO_LONG_PRESS,
    SOURCE_LONG_PRESS,
    UP_LONG_PRESS,
    DOWN_LONG_PRESS,
} Button;

Button buttons_get_pressed();

#endif //APP_TEMPLATE_BUTTONS_H
