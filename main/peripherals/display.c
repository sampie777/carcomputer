//
// Created by samuel on 17-7-22.
//

#include <stdio.h>
#include "display.h"

void display_init() {
    printf("Initializing display...\n");
}

void display_update(State *state) {
    printf("Speed: %d\tWiFi: %s\tBluetooth: %s\n",
           state->car.speed,
           state->wifi.connected ? "connected" : "not connected",
           state->bluetooth.connected ? "connected" : "not connected");
}