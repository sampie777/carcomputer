//
// Created by samuel on 17-7-22.
//

#ifndef APP_TEMPLATE_STATE_H
#define APP_TEMPLATE_STATE_H

#include <esp_http_client.h>
#include <stdbool.h>
#include "config.h"
#include "peripherals/gpsgsm/definitions.h"
#include "math.h"

typedef enum {
    Screen_Booting = 0,
    Screen_Rebooting,
    Screen_Menu,
    Screen_Speed,
    Screen_Sensors,
    Screen_Actions,
    Screen_ActivateDiagnostics,
    Screen_About,
} Screen;

typedef enum {
    ScreenSensors_SensorsMotionValues = 0,
    ScreenSensors_SensorsMotionGraphical,
    ScreenSensors_SensorsInputs,
    ScreenSensors_GPS,
    ScreenSensors_MAX_VALUE,
} SubScreenSensors;

typedef enum {
    ScreenAbout_SD = 0,
    ScreenAbout_AboutCruiseControl,
    ScreenAbout_AboutCar,
    ScreenAbout_MAX_VALUE,
} SubScreenAbout;

typedef enum {
    SubScreenSpeed_CruiseControl = 0,
    SubScreenSpeed_PedalControl,
    SubScreenSpeed_Graph,
    SubScreenSpeed_MAX_VALUE,
} SubScreenSpeed;

typedef enum {
    SubScreenCruiseControl_Main = 0,
    SubScreenCruiseControl_Graph,
    SubScreenCruiseControl_ETA,
    SubScreenCruiseControl_MAX_VALUE,
} SubScreenCruiseControl;

typedef struct {
    SubScreenSensors sensors;
    SubScreenAbout about;
    SubScreenSpeed speed;
    SubScreenCruiseControl cruise_control;
} SubScreen;

// This also determines the order in which the options are shown on the display
typedef enum {
    ScreenMenuOption_CruiseControl,
    ScreenMenuOption_Sensors,
    ScreenMenuOption_Actions,
    ScreenMenuOption_About,
    ScreenMenuOption_MAX_VALUE, ScreenMenuOption_GPS,
} MainMenuScreenOptions;

typedef enum {
    ScreenActionsOptions_LockDoors,
    ScreenActionsOptions_ActivateSeatbelt,
    ScreenActionsOptions_ActivateDiagnostics,
    ScreenActionsOptions_ActivateSim,
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
    SubScreen subscreen;
} DisplayState;

#define CRUISE_CONTROL_GRAPH_SIZE DISPLAY_WIDTH

typedef struct {
    int virtual_gas_pedal[CRUISE_CONTROL_GRAPH_SIZE]; // The value will be stored in a int between 0 and 100, to reduce unnecessary memory usage
} CruiseControlGraphState;

typedef struct {
    bool enabled;
    double previous_target_speed; // Absolute value in km/h. Used for resetting the CC to the last used target speed after disconnecting or whatever
    double target_speed;          // Absolute value in km/h
    double eta_target_speed;      // Absolute value in km/h
    double initial_control_value; // Relative value between 0.0 and 1.0
    double control_value;         // Relative value between 0.0 and 1.0
    double pidKp;
    double pidKi;
    double pidKd;
    double integral;
    double derivative;
    double error;

    CruiseControlGraphState graph;
} CruiseControlState;

typedef struct {
    bool enabled;
    double target_value;
    double previous_value;
} PedalControlState;

typedef struct {
    double virtual_gas_pedal; // Relative value between 0.0 and 1.0
    CruiseControlState cruise_control;
    PedalControlState pedal_control;
} SpeedControlState;

typedef struct {
    double speed;        // Absolute value in km/h
    double acceleration; // Value in m/s2
    double rpm;          // Absolute value in rpm
    uint16_t rpm_raw;
    int64_t last_can_message_time;
    uint32_t odometer_start;
    uint32_t odometer;
    CarGearPosition estimated_gear;

    bool gas_pedal_connected;
    double gas_pedal_0_min_value_volts; // Absolute value in Volts
    double gas_pedal_1_min_value_volts; // Absolute value in Volts
    double gas_pedal_0_max_value_volts; // Absolute value in Volts
    double gas_pedal_1_max_value_volts; // Absolute value in Volts
    double gas_pedal;                   // Relative value between 0.0 and 1.0
    double gas_pedal_0_volts;           // Current value in absolute Volts
    double gas_pedal_1_volts;           // Current value in absolute Volts

    // Use bit fields to reduce memory usage
    bool is_connected: 1;
    bool is_controller_connected: 1;
    bool is_braking: 1;
    bool is_ignition_on: 1;
    bool is_in_reverse: 1;
    bool is_drivers_door_locked: 1;
    bool is_other_doors_locked: 1;
    bool is_blower_on: 1;
    bool is_locked: 1;
    bool is_parking_brake_on: 1;
    bool is_seatbelt_on: 1;

    bool should_be_locked;
} CarState;

typedef struct {
    bool connected;
    bool has_compass;
    Vector3 bias;
    double rotation_matrix[3][3];
    double accel_x;
    double accel_y;
    double accel_z;
    Vector3Spherical accel;
    double gyro_x;
    double gyro_y;
    double gyro_z;
    Vector3Spherical gyro;
    double compass_x;
    double compass_y;
    double compass_z;
    Vector3Spherical compass;
    double temperature;
} MotionState;

typedef struct {
    bool is_connected;
    char filename[SD_PATH_MAX_LENGTH];
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
    double altitude;     // m
    double ground_speed; // km/h
    double ground_heading;

    Time time;
    int64_t gngga_last_updated;
    int64_t gnrmc_last_updated;
} GpsState;

typedef struct {
    bool is_uploading;
    esp_http_client_method_t request_type;
    int64_t upload_start_time;
    SmsState sms_state;

    Time time;
    enum SimStatus sim_status;
} GsmState;

typedef enum {
    DiagnosticsStep_Off = 0,
    DiagnosticsStep_IgnitionOff,
    DiagnosticsStep_IgnitionOffWait5Sec,
    DiagnosticsStep_IgnitionOn,
    DiagnosticsStep_IgnitionOnWait3Sec,
    DiagnosticsStep_DepressPedal5Times,
    DiagnosticsStep_OnWait7Sec,
    DiagnosticsStep_DepressPedal10Sec,
    DiagnosticsStep_ReleasePedal,
} DiagnosticsStepStatus;

typedef struct {
    DiagnosticsStepStatus status;
    int64_t process_start_time;
    int64_t process_estimated_end_time;
} Diagnostics;

typedef struct {
    int button0;
    int button1;
} ButtonsState;

typedef struct {
    bool is_booting;
    bool is_rebooting;
    int max_progress;
    int progress;
} BootState;

typedef struct {
    int16_t power_off_count_down_sec;
    uint32_t logging_session_id;
    uint32_t errors;
    char *device_name;
    CarState car;
    SpeedControlState speed_control;
    DisplayState display;
    MotionState motion;
    SDState storage;
    GpsState location;
    GsmState gsm;
    A9GState a9g;
    Diagnostics diagnostics;
    ButtonsState buttons;
    BootState boot;
} State;

#endif //APP_TEMPLATE_STATE_H
