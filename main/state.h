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

typedef enum {
    Screen_Booting = 0,
    Screen_Rebooting,
    Screen_Registration,
    Screen_Menu,
    Screen_CruiseControl,
    Screen_Sensors,
    Screen_GPS,
    Screen_Config,
} Screen;

typedef enum {
    ScreenMenuOption_CruiseControl,
    ScreenMenuOption_Sensors,
    ScreenMenuOption_GPS,
    ScreenMenuOption_Config,
    ScreenMenuOption_MAX_VALUE,
} ScreenMenuOptions;

typedef enum {
    GearNeutral = 0,
    GearReverse,
    Gear1,
    Gear2,
    Gear3,
    Gear4,
    Gear5,
} CarGearPosition;

typedef struct {
    Screen current_screen;
    ScreenMenuOptions menu_option_selection;
} DisplayState;

typedef struct {
    bool enabled;
    double target_speed;            // Absolute value in km/h
    double virtual_gas_pedal;       // Relative value between 0.0 and 1.0
    double initial_control_value;   // Relative value between 0.0 and 1.0
    double control_value;           // Relative value between 0.0 and 1.0
} CruiseControlState;

typedef struct {
    bool is_connected;
    bool is_controller_connected;
    bool is_braking;
    bool is_ignition_on;
    bool is_in_reverse;
    double speed;                   // Absolute value in km/h
    double rpm;                     // Absolute value in rpm
    int64_t last_can_message_time;
    uint32_t odometer_start;
    uint32_t odometer;
    CarGearPosition estimated_gear;

    bool gas_pedal_connected;
    double gas_pedal;               // Relative value between 0.0 and 1.0
    uint16_t gas_pedal_0_min_value;      // Absolute value between 0 and ADC max
    uint16_t gas_pedal_1_min_value;      // Absolute value between 0 and ADC max

    bool is_drivers_door_open;
    bool is_other_doors_open;
    bool is_blower_on;
    bool is_locked;
} CarState;

typedef struct {
    esp_ip4_addr_t ip;
    char ssid[32];
    bool is_scanning;
    bool has_scan_results;
    bool is_connecting;
    bool is_connected;
} WiFiState;

typedef struct {
    bool connected;
} BluetoothState;

typedef struct {
    bool connected;
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
    bool is_connected;
    char filename[32];
} SDState;


typedef struct {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    int8_t timezone;
    uint8_t day;
    uint8_t month;
    uint16_t year;
} Time;

typedef struct {
    bool is_gps_on;
    uint8_t quality;
    uint8_t satellites;
    bool is_effective_positioning;
    double latitude;
    double longitude;
    double altitude;        // m
    double ground_speed;    // km/h
    double ground_heading;

    Time time;
    int64_t gngga_last_updated;
    int64_t gnrmc_last_updated;
} GpsState;

typedef struct {
    bool is_uploading;
    esp_http_client_method_t request_type;
    int64_t upload_start_time;

    Time time;
} GsmState;

typedef struct {
    bool is_authenticated;
    bool should_authenticate;
    bool is_registration_status_check_in_process;
    char *registration_token;
    char *access_token;
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
    DisplayState display;
    bool is_booting;
    bool is_rebooting;
    int16_t power_off_count_down_sec;
    bool server_is_uploading;
    bool trip_has_been_uploaded;
    uint32_t logging_session_id;
    uint32_t errors;
    char *device_name;
} State;

#endif //APP_TEMPLATE_STATE_H
