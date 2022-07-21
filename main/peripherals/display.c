//
// Created by samuel on 17-7-22.
//

#include <stdio.h>
#include <freertos/portmacro.h>
#include <string.h>
#include "display.h"
#include "../config.h"

void show_error_message(State *state){
    printf("Core: %d\tError message: %s\n", xPortGetCoreID(), state->display.error_message);
}

void display_init() {
    printf("Initializing display...\n");
}

void display_update(State *state) {
    if (esp_timer_get_time() < state->display.last_error_message_time + DISPLAY_ERROR_MESSAGE_TIME) {
        return show_error_message(state);
    }

    printf("Core: %d\tSpeed: %3.1f\tWiFi: %s\tBluetooth: %s\n",
           xPortGetCoreID(),
           state->car.speed,
           state->wifi.connected ? "connected" : "not connected",
           state->bluetooth.connected ? "connected" : "not connected");
}

void display_set_error_message(State *state, char *message) {
    strncpy(state->display.error_message, message, DISPLAY_ERROR_MESSAGE_MAX_LENGTH);
    state->display.last_error_message_time = esp_timer_get_time();
}