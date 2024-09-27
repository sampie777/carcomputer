//
// Created by samuel on 2024/09/27.
//

#ifndef LED_H
#define LED_H

#include <stdbool.h>

void led_init();

void led_set(bool value);

inline void led_set_on() { led_set(true); }
inline void led_set_off() { led_set(false); }

void led_blink(unsigned long interval);

#endif //LED_H
