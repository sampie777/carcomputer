//
// Created by samuel on 17-7-22.
//

#ifndef APP_TEMPLATE_STATE_H
#define APP_TEMPLATE_STATE_H

#include <esp_netif_ip_addr.h>
#include <stdbool.h>
#include <esp_http_client.h>
#include "config.h"
#include "peripherals/gpsgsm/definitions.h"

typedef struct {
    uint8_t enabled;
    double target_speed;            // Absolute value in km/h
    double virtual_gas_pedal;       // Relative value between 0.0 and 1.0
    double initial_control_value;   // Relative value between 0.0 and 1.0
    double control_value;           // Relative value between 0.0 and 1.0
} CruiseControlState;

typedef struct {
    uint8_t is_connected;
    uint8_t is_controller_connected;
    uint8_t is_braking;
    uint8_t is_ignition_on;
    double speed;                   // Absolute value in km/h
    double rpm;                     // Absolute value in rpm
    int64_t last_can_message_time;
    uint32_t odometer_start;
    uint32_t odometer;

    uint8_t gas_pedal_connected;
    double gas_pedal;               // Relative value between 0.0 and 1.0
    uint16_t gas_pedal_0_min_value;      // Absolute value between 0 and ADC max
    uint16_t gas_pedal_1_min_value;      // Absolute value between 0 and ADC max
} CarState;

typedef struct {
    esp_ip4_addr_t ip;
    char ssid[32];
    uint8_t is_scanning;
    uint8_t has_scan_results;
    uint8_t is_connecting;
    uint8_t is_connected;
} WiFiState;

typedef struct {
    uint8_t connected;
} BluetoothState;

typedef struct {
    uint8_t connected;
    double accel_x;
    double accel_y;
    double accel_z;
    double gyro_x;
    double gyro_y;
    double gyro_z;
    double compass_x;
    double compass_y;
    double compass_z;
    double temperature;
} MotionState;

typedef struct {
    uint8_t is_connected;
    char filename[32];
} SDState;

typedef struct {
    uint8_t is_gps_on;
    uint8_t quality;
    uint8_t satellites;
    uint8_t is_effective_positioning;
    double latitude;
    double longitude;
    double altitude;        // m
    double ground_speed;    // km/h
    double ground_heading;

    struct Time {
        uint8_t seconds;
        uint8_t minutes;
        uint8_t hours;
        uint8_t timezone;
        uint8_t day;
        uint8_t month;
        uint16_t year;
    } time;
} GpsState;

typedef struct {
    uint8_t is_uploading;
    esp_http_client_method_t request_type;
    int64_t upload_start_time;
} GsmState;

typedef struct {
    uint8_t is_authenticated;
    uint8_t should_authenticate;
    int64_t is_registration_status_check_in_process;
    char *registration_token;
    char *access_code;
} ServerState;

typedef struct {
    CarState car;
    WiFiState wifi;
    BluetoothState bluetooth;
    CruiseControlState cruise_control;
    MotionState motion;
    SDState storage;
    GpsState location;
    GsmState gsm;
    A9GState a9g;
    ServerState server;
    uint8_t is_booting;
    uint8_t is_rebooting;
    int16_t power_off_count_down_sec;
    uint8_t server_is_uploading;
    uint8_t trip_has_been_uploaded;
    uint32_t errors;
} State;

#endif //APP_TEMPLATE_STATE_H
