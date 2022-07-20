//
// Created by samuel on 17-7-22.
//

#include <stdio.h>
#include <freertos/portmacro.h>
#include "display.h"

void display_init() {
    printf("Initializing display...\n");
}

void display_update(State *state) {
    printf("Core: %d\tSpeed: %3.1f\tWiFi: %s\tBluetooth: %s\n",
           xPortGetCoreID(),
           state->car.speed,
           state->wifi.connected ? "connected" : "not connected",
           state->bluetooth.connected ? "connected" : "not connected");
}