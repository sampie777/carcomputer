//
// Created by Samuel-Anton Jansen on 2025/11/13.
//

#include "task.h"

#include <stdio.h>

#include "../../../../test/utils/common.h"


void *_update_function = NULL;

void set_update_function(void *function) {
    _update_function = function;
}

void vTaskDelay(const TickType_t xTicksToDelay) {
    step_time(xTicksToDelay * 10);

    if (_update_function != NULL) {
        printf("Updagint\n");
        ((void(*)()) _update_function)();
    }
};
