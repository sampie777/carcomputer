#include <sys/cdefs.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "state.h"
#include "utils.h"
#include "control.h"
#include "connectivity/i2c.h"
#include "connectivity/spi.h"
#include "connectivity/wifi.h"
#include "connectivity/bluetooth.h"
#include "peripherals/display.h"
#include "peripherals/canbus.h"

_Noreturn void process_gui(void *args) {
    State *state = args;
    display_init();

    while (1) {
        display_update(state);
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

_Noreturn void process_main(State *state) {
    i2c_init();
    spi_init(state);
    canbus_init(state);
    bluetooth_init(state);

    wifi_connect(state);

    state->is_booting = false;
    state->display.is_dirty = true;

    while (1) {
        // Collect data
        control_read_can_bus(state);
        control_read_analog_sensors(state);

        // Process data
        control_door_lock(state);
        control_cruise_control(state);

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void init() {
    nvs_init();
}

// Running on main core
void app_main(void) {
    State state = {
            .is_booting = true,
            .display.is_dirty = true,
    };

    init();

    // Init second core
    portBASE_TYPE result = xTaskCreatePinnedToCore(&process_gui, "process_gui",
                                                   3584 + 512, &state,
                                                   0, NULL, 1);
    if (result != pdTRUE) {
        printf("Failed to create task for second core\n");
    }

    process_main(&state);
}
