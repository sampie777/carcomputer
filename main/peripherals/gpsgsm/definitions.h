//
// Created by samuel on 15-11-22.
//

#ifndef CARCOMPUTER_DEFINITIONS_H
#define CARCOMPUTER_DEFINITIONS_H

#include <stdbool.h>

#define A9G_INIT "Init..."
#define A9G_CGATT_DISABLE "AT+CGATT=0"
#define A9G_CGATT_ENABLE "AT+CGATT=1"
#define A9G_CGDCONT_DISABLE "AT+CGDCONT=0"
#define A9G_CGDCONT_ENABLE "AT+CGDCONT=1,\"IP\",\"internet\",\"0.0.0.0\",0,0"
#define A9G_CGACT_PNP_DISABLE "AT+CGACT=0"
#define A9G_CGACT_PNP_ENABLE "AT+CGACT=1,1"
#define A9G_AGPS_DISABLE "AT+AGPS=0"
#define A9G_AGPS_ENABLE "AT+AGPS=1"
#define A9G_GPS_ENABLE "AT+GPS=1"
#define A9G_GPS_DISABLE "AT+GPS=0"
#define A9G_GPSRD_ENABLE "AT+GPSRD=1"
#define A9G_GET_SIGNAL_QUALITY "AT+CSQ" // 0-31 signal strength, https://m2msupport.net/m2msupport/atcsq-signal-quality/
#define A9G_RESET "AT+RST=1"

typedef struct {
    double timestamp;
    double latitude;
    char latitude_direction;
    double longitude;
    char longitude_direction;
    int quality;
    int satellites;
    double hdop;
    double altitude;
    char unit;
    double geoidal_separation;
    char geoidal_separation_unit;
    double correction_age;
    int station_id;
    unsigned int checksum;
} NmeaGNGGAMessage;

typedef struct {
    double timestamp;
    char status;
    double latitude;
    char latitude_direction;
    double longitude;
    char longitude_direction;
    double ground_speed; // knots
    double ground_heading;
    int date;
    double declination;
    char declination_direction;
    char mode;
    unsigned int checksum;
} NmeaGNRMCMessage;

typedef struct {
    // Use bit fields to reduce memory usage
    bool initialized: 1;
    bool network_attached: 1;
    bool pnp_parameters_set: 1;
    bool pnp_activated: 1;
    bool agps_enabled: 1;
    bool gps_enabled: 1;
    bool gps_logging_enabled: 1;
    bool gps_logging_started: 1;
    uint8_t network_error_count;
} A9GState;

typedef enum {
    Idle = 0,
    Sending,
} SmsState;

enum A9GCommand {
    A9GCommand_Skip,
    A9GCommand_CGATT_Disable,
    A9GCommand_CGATT_Enable,
    A9GCommand_CGACT_PNP_Disable,
    A9GCommand_CGACT_PNP_Enable,
    A9GCommand_CGDCONT_Disable,
    A9GCommand_CGDCONT_Enable,
    A9GCommand_AGPS_Disable,
    A9GCommand_AGPS_Enable,
    A9GCommand_GPSRD_Enable,
    A9GCommand_Reset_Software,
};

enum SimStatus {
    SIM_UNKNOWN,
    SIM_NOT_PRESENT,
    SIM_PRESENT,
};

#endif //CARCOMPUTER_DEFINITIONS_H
