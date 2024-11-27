#include <driver/adc.h>

#include "freertos/FreeRTOS.h"
#include "config.h"
#include "state.h"
#include "tasks/task_primary.h"
#include "tasks/task_secondary.h"

#define PRIMARY_TASK_STACK_SIZE 32000
#define SECONDARY_TASK_STACK_SIZE 8000

// Setting up the different tasks
void app_main(void) {
    static State state = {0};
    state.is_booting = true;
    state.power_off_count_down_sec = -1;
    state.cruise_control.pidKp = CRUISE_CONTROL_PID_Kp;
    state.cruise_control.pidKi = CRUISE_CONTROL_PID_Ki;
    state.cruise_control.pidKd = CRUISE_CONTROL_PID_Kd;

    xTaskCreatePinnedToCore(
        task_primary,
        "task_primary",
        PRIMARY_TASK_STACK_SIZE,
        &state,
        2,
        NULL,
        0);

    xTaskCreatePinnedToCore(
        task_secondary,
        "task_secondary",
        SECONDARY_TASK_STACK_SIZE,
        &state,
        1,
        NULL,
        1);
}
