//
// Created by samuel on 17-7-22.
//

#include <stdio.h>
#include "display.h"

void display_init() {
    printf("initizizing GUI...\n");
}

void display_update(State *state) {
    printf("Speed: %d\n", state->car.speed);
}