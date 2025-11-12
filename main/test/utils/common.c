//
// Created by Samuel-Anton Jansen on 2025/11/12.
//

#include "common.h"
#include "../mocks/idf/esp_timer.h"
#include "../../utils.h"

void step_time() { _esp_timer_set_time((esp_timer_get_time_ms() + STEP) * 1000); }