//
// Created by samuel on 17-7-22.
//

#ifndef APP_TEMPLATE_WIFI_H
#define APP_TEMPLATE_WIFI_H

#include <esp_wifi_types.h>
#include "../state.h"

typedef struct {
    uint8_t ssid[32];
    uint8_t password[64];
    wifi_auth_mode_t authmode;
} WiFiAPCredentials;

void wifi_scan(State *state);

void wifi_init(State *state);

#endif //APP_TEMPLATE_WIFI_H
