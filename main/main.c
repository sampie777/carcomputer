#include "freertos/FreeRTOS.h"
#include "config.h"
#include "state.h"
#include "version.h"
#include "tasks/task_primary.h"
#include "tasks/task_secondary.h"

#define PRIMARY_TASK_STACK_SIZE (32000)
#define SECONDARY_TASK_STACK_SIZE (8000)

// Setting up the different tasks
void app_main(void) {
    printf("APP_VERSION %s\n", APP_VERSION);

    static State state = {0};
    state.boot.is_booting = true;
    state.power_off_count_down_sec = -1;
    state.cruise_control.pidKp = CRUISE_CONTROL_PID_Kp;
    state.cruise_control.pidKi = CRUISE_CONTROL_PID_Ki;
    state.cruise_control.pidKd = CRUISE_CONTROL_PID_Kd;
    state.device_name = "Default";
    state.location.time.timezone = 2;    // GMT+2
    state.motion.bias.x = 1.1;
    state.motion.bias.y = 0.1;
    state.motion.bias.z = 0.10;

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
