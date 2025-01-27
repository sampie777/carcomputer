//
// Created by samuel on 17-7-22.
//

#ifndef APP_TEMPLATE_STATE_H
#define APP_TEMPLATE_STATE_H

#include <esp_http_client.h>
#include <stdbool.h>
#include "peripherals/gpsgsm/definitions.h"

typedef enum {
    Screen_Booting = 0,
    Screen_Rebooting,
    Screen_Menu,
    Screen_CruiseControl,
    Screen_Sensors,
    Screen_Actions,
    Screen_GPS,
    Screen_ErrorCodes,
} Screen;

// This also determines the order in which the options are shown on the display
typedef enum {
    ScreenMenuOption_CruiseControl,
    ScreenMenuOption_Sensors,
    ScreenMenuOption_GPS,
    ScreenMenuOption_Actions,
    ScreenMenuOption_MAX_VALUE,
} MainMenuScreenOptions;

typedef enum {
    ScreenActionsOptions_LockDoors,
    ScreenActionsOptions_ErrorCodes,
    ScreenActionsOptions_Reboot,
    ScreenActionsOptions_MAX_VALUE,
} ActionsScreenOptions;

typedef enum {
    GearReverse = -1,
    GearNeutral,
    Gear1,
    Gear2,
    Gear3,
    Gear4,
    Gear5,
} CarGearPosition;

typedef struct {
    Screen current_screen;
    MainMenuScreenOptions menu_option_selection;
    ActionsScreenOptions actions_option_selection;
} DisplayState;

typedef struct {
    bool enabled;
    double previous_target_speed;   // Absolute value in km/h. Used for resetting the CC to the last used target speed after disconnecting or whatever
    double target_speed;            // Absolute value in km/h
    double virtual_gas_pedal;       // Relative value between 0.0 and 1.0
    double initial_control_value;   // Relative value between 0.0 and 1.0
    double control_value;           // Relative value between 0.0 and 1.0
    double pidKp;
    double pidKi;
    double pidKd;
} CruiseControlState;

typedef struct {
    bool is_connected;
    bool is_controller_connected;
    bool is_braking;
    bool is_ignition_on;
    bool is_in_reverse;
    double speed;                   // Absolute value in km/h
    double rpm;                     // Absolute value in rpm
    uint16_t rpm_raw;
    int64_t last_can_message_time;
    uint32_t odometer_start;
    uint32_t odometer;
    CarGearPosition estimated_gear;

    bool gas_pedal_connected;
    double gas_pedal_0_min_value_volts;      // Absolute value in Volts
    double gas_pedal_1_min_value_volts;      // Absolute value in Volts
    double gas_pedal_0_max_value_volts;      // Absolute value in Volts
    double gas_pedal_1_max_value_volts;      // Absolute value in Volts
    double gas_pedal;               // Relative value between 0.0 and 1.0
    double gas_pedal_0_volts;       // Current value in absolute Volts
    double gas_pedal_1_volts;       // Current value in absolute Volts

    bool is_drivers_door_open;
    bool is_other_doors_open;
    bool is_blower_on;
    bool is_locked;
    bool is_parking_brake_on;
    bool is_seatbelt_on;
} CarState;

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

typedef enum {
    ErrorCodes_Off = 0,
    ErrorCodes_IgnitionOff,
    ErrorCodes_IgnitionOffWait3Sec,
    ErrorCodes_IgnitionOn,
    ErrorCodes_IgnitionOnWait3Sec,
    ErrorCodes_DepressPedal5Times,
    ErrorCodes_OnWait7Sec,
    ErrorCodes_DepressPedal10Sec,
    ErrorCodes_ReleasePedal,
} ErrorCodesStatus;

typedef struct {
    ErrorCodesStatus status;
    int64_t wait_timer_end;
} ErrorCodes;

typedef struct {
    bool is_booting;
    bool is_rebooting;
    int16_t power_off_count_down_sec;
    uint32_t logging_session_id;
    uint32_t errors;
    char* device_name;
    CarState car;
    CruiseControlState cruise_control;
    DisplayState display;
    MotionState motion;
    SDState storage;
    GpsState location;
    GsmState gsm;
    A9GState a9g;
    ErrorCodes error_codes;
} State;

#endif //APP_TEMPLATE_STATE_H
