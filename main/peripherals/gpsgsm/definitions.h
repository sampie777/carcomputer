//
// Created by samuel on 15-11-22.
//

#ifndef CARCOMPUTER_DEFINITIONS_H
#define CARCOMPUTER_DEFINITIONS_H

#define A9G_CGATT_DISABLE "AT+CGATT=0\r"
#define A9G_CGATT_ENABLE "AT+CGATT=1\r"
#define A9G_CGDCONT_DISABLE "AT+CGDCONT=0\r"
#define A9G_CGDCONT_ENABLE "AT+CGDCONT=1,\"IP\",\"internet\",\"0.0.0.0\",0,0\r"
#define A9G_CGACT_PNP_DISABLE "AT+CGACT=0\r"
#define A9G_CGACT_PNP_ENABLE "AT+CGACT=1,1\r"
#define A9G_AGPS_DISABLE "AT+AGPS=0\r"
#define A9G_AGPS_ENABLE "AT+AGPS=1\r"
#define A9G_GPSRD_ENABLE "AT+GPSRD=1\r"
#define A9G_GET_SIGNAL_QUALITY "AT+CSQ\r"
#define A9G_RESET "AT+RST=1\r"

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
    double ground_speed;    // knots
    double ground_heading;
    int date;
    double declination;
    char declination_direction;
    char mode;
    unsigned int checksum;
} NmeaGNRMCMessage;

enum A9Status {
    A9Status_Unknown = 0,
    A9Status_Requested,
    A9Status_Ok,
    A9Status_Error,
    A9Status_Disabled,
};

typedef struct {
    enum A9Status initialized;
    enum A9Status network_attached;
    enum A9Status pnp_parameters_set;
    enum A9Status pnp_activated;
    enum A9Status agps_enabled;
    enum A9Status gps_logging_enabled;
    bool gps_logging_started;
    uint8_t network_error_count;
} A9GState;

enum SmsState {
    Idle = 0,
    Sending,
    SentSuccess,
    SentFailed
};

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

#endif //CARCOMPUTER_DEFINITIONS_H
