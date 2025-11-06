//
// Created by Samuel-Anton Jansen on 2025/11/06.
//

#include "esp_timer.h"

int64_t _esp_time = 0;

int64_t esp_timer_get_time(void) { return _esp_time; }

void _esp_timer_set_time(int64_t value) { _esp_time = value; }
