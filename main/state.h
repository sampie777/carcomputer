//
// Created by samuel on 17-7-22.
//

#ifndef APP_TEMPLATE_STATE_H
#define APP_TEMPLATE_STATE_H

#include <stdint.h>
#include <stdbool.h>

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
    bool enabled;
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
} CarState;

typedef struct {
    CarState car;
    CruiseControlState cruise_control;
    bool is_booting;
    bool is_rebooting;
    int16_t power_off_count_down_sec;
    uint32_t logging_session_id;
    uint32_t errors;
} State;

#endif //APP_TEMPLATE_STATE_H
