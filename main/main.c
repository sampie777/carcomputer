#include <sys/cdefs.h>
#include <stdio.h>
#include <driver/adc.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "state.h"
#include "utils.h"
#include "control/control.h"
#include "connectivity/i2c.h"
#include "connectivity/spi.h"
#if WIFI_ENABLE
#include "connectivity/wifi.h"
#endif
#if BLUETOOTH_ENABLE
#include "connectivity/bluetooth.h"
#endif
#include "peripherals/display/display.h"

_Noreturn void process_gui(void *args) {
    State *state = args;
    display_init();

    while (1) {
        display_update(state);
    }
    vTaskDelete(NULL);
}

_Noreturn void process_main(State *state) {
    adc1_config_width(ADC_RESOLUTION - 9);
    i2c_init();
    spi_init(state);
    control_init(state);
#if BLUETOOTH_ENABLE
    bluetooth_init(state);
#endif

#if WIFI_ENABLE
    wifi_connect(state);
#endif

    state->is_booting = false;

    while (1) {
        // Collect data
        control_read_can_bus(state);
        control_read_analog_sensors(state);
        control_read_user_input(state);

        // Process data
        control_door_lock(state);
        control_cruise_control(state);
    }
}

void init() {
    nvs_init();
}

// Running on main core
void app_main(void) {
    State state = {
            .is_booting = true,
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
