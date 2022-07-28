//
// Created by samuel on 17-7-22.
//

#ifndef APP_TEMPLATE_STATE_H
#define APP_TEMPLATE_STATE_H

#include <esp_netif_ip_addr.h>
#include <stdbool.h>
#include "config.h"

typedef struct {
    uint8_t connected;
    double speed;                   // Absolute value in km/h
    double rpm;                     // Absolute value in rpm
    uint8_t is_braking;
    unsigned long last_can_message_time;

    // Cruise control
    uint8_t cruise_control_enabled;
    uint8_t gas_pedal_connected;
    double target_speed;            // Absolute value in km/h
    double gas_pedal;               // Relative value between 0.0 and 1.0
    double virtual_gas_pedal;       // Relative value between 0.0 and 1.0
    double initial_control_value;   // Relative value between 0.0 and 1.0
    double control_value;           // Relative value between 0.0 and 1.0
    int gas_pedal_0_min_value;      // Absolute value between 0 and ADC max
    int gas_pedal_1_min_value;      // Absolute value between 0 and ADC max
} CarState;

typedef struct {
    uint8_t connected;
    esp_ip4_addr_t ip;
} WiFiState;

typedef struct {
    uint8_t connected;
} BluetoothState;

typedef struct {
    char error_message[DISPLAY_ERROR_MESSAGE_MAX_LENGTH + 1];
    unsigned long last_error_message_time;
    uint8_t is_dirty;
} DisplayState;

typedef struct {
    CarState car;
    WiFiState wifi;
    BluetoothState bluetooth;
    DisplayState display;
    uint8_t is_booting;
} State;

#endif //APP_TEMPLATE_STATE_H
