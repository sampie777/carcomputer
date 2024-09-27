#include <sys/cdefs.h>
#include <stdio.h>
#include <driver/adc.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "state.h"
#include "control/control.h"
#include "connectivity/i2c.h"
#include "connectivity/spi.h"
#include "peripherals/display/display.h"
#include "peripherals/gpsgsm/gpsgsm.h"
#include "control/data_logger.h"
#include "control/security.h"
#include "backend/server.h"
#include "utils/nvs.h"
#include "return_codes.h"

#if WIFI_ENABLE
#include "connectivity/wifi.h"
#endif
#if BLUETOOTH_ENABLE
#include "connectivity/bluetooth.h"
#endif


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
    security_init();
    control_init(state);
    gpsgsm_init(&state->a9g);
#if BLUETOOTH_ENABLE
    bluetooth_init(state);
#endif
#if WIFI_ENABLE
    wifi_init(state);
#endif
    server_init(state);
    data_logger_init(state);

    state->is_booting = false;

    while (1) {
        // Collect data
#if SWITCH_0_READ_CAN_BUS
        control_read_can_bus(state);
#endif
#if SWITCH_1_READ_INPUTS
        control_read_analog_sensors(state);
        control_read_user_input(state);
#endif
#if SWITCH_2_PROCESS_GSMGPS
        gpsgsm_process(state);

        // Enable this line for sending a test SMS, like for keeping the SIM card active
//        control_send_test_sms(state);

#endif
#if SWITCH_3_SET_OUTPUTS
        // Process data
        security_step(state);
        control_mpu_power(state);
        control_door_lock(state);
#endif
#if SWITCH_4_TRIP_END
        control_trip_logger(state);
#endif
#if SWITCH_5_CRUISE_CONTROL_AND_CRASH_DETECTION
        control_cruise_control(state);
        control_crash_detection(state);
#endif
#if SWITCH_6_CAR_GEAR
        control_car_gear(state);

#endif
#if SWITCH_7_TRIP_LOGGER
        data_logger_process(state);

#endif
#if SWITCH_8_AUTH
#if WIFI_ENABLE
        wifi_scan(state);
#endif
        server_process(state);
#endif
    }
}

void init(State *state) {
    nvs_init();

    size_t length;
    int result = nvs_read_device_name(&(state->device_name), &length);
    if (length == 0 || result != RESULT_OK) {
        state->device_name = "Unregistered";
    }
}

// Running on main core
__attribute__((unused)) void app_main(void) {
    State state = {
            .is_booting = true,
            .power_off_count_down_sec = -1,
            .location.time.timezone = 2,    // GMT+2
            .device_name = "Unregistered"
    };

    init(&state);

    // Init second core
    portBASE_TYPE result = xTaskCreatePinnedToCore(&process_gui, "process_gui",
                                                   3584 + 512, &state,
                                                   0, NULL, 1);
    if (result != pdTRUE) {
        printf("Failed to create task for second core\n");
    }

    process_main(&state);
}
