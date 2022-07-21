//
// Created by samuel on 17-7-22.
//

#ifndef APP_TEMPLATE_STATE_H
#define APP_TEMPLATE_STATE_H

#include <esp_netif_ip_addr.h>
#include "config.h"

typedef struct {
    int connected;
    double speed;
    double rpm;
    int isBraking;
    double gas_pedal;
    double virtual_gas_pedal;
    unsigned long last_can_message_time;
} CarState;

typedef struct {
    int connected;
    esp_ip4_addr_t ip;
} WiFiState;

typedef struct {
    int connected;
} BluetoothState;

typedef struct {
    char error_message[DISPLAY_ERROR_MESSAGE_MAX_LENGTH];
    unsigned long last_error_message_time;
} DisplayState;

typedef struct {
    CarState car;
    WiFiState wifi;
    BluetoothState bluetooth;
    DisplayState display;
} State;

#endif //APP_TEMPLATE_STATE_H
