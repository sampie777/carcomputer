//
// Created by samuel on 17-7-22.
//

#ifndef APP_TEMPLATE_STATE_H
#define APP_TEMPLATE_STATE_H

#include <esp_netif_ip_addr.h>

typedef struct {
    int speed;
} CarState;

typedef struct {
    int connected;
    esp_ip4_addr_t ip;
} WiFiState;

typedef struct {
    int connected;
} BluetoothState;

typedef struct {
    CarState car;
    WiFiState wifi;
    BluetoothState bluetooth;
} State;

#endif //APP_TEMPLATE_STATE_H
