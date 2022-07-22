//
// Created by samuel on 17-7-22.
//

#include <stdio.h>
#include <freertos/portmacro.h>
#include <string.h>
#include "../config.h"
#include "display.h"

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

    printf("Speed: %3.1f\tWiFi: [%s] Bluetooth: [%s] CAN: [%s]\n",
           state->car.speed,
           state->wifi.connected ? "x" : " ",
           state->bluetooth.connected ? "x" : " ",
           state->car.connected ? "x" : " ");
}

void display_set_error_message(State *state, char *message) {
    strncpy(state->display.error_message, message, DISPLAY_ERROR_MESSAGE_MAX_LENGTH);
    state->display.last_error_message_time = esp_timer_get_time();
}