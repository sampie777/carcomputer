//
// Created by Samuel-Anton Jansen on 2025/11/06.
//

#ifndef CARCOMPUTER_ESP_TIMER_H
#define CARCOMPUTER_ESP_TIMER_H
#include <stdint.h>

int64_t esp_timer_get_time(void);
void _esp_timer_set_time(int64_t value);

#endif //CARCOMPUTER_ESP_TIMER_H