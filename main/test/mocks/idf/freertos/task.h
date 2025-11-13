//
// Created by Samuel-Anton Jansen on 2025/11/06.
//

#ifndef CARCOMPUTER_TASK_H
#define CARCOMPUTER_TASK_H
#include <stdint.h>

typedef uint32_t TickType_t;
void vTaskDelay( const TickType_t xTicksToDelay );
void set_update_function(void *function);

#endif //CARCOMPUTER_TASK_H