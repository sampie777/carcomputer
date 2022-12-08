//
// Created by samuel on 16-11-22.
//

#ifndef CARCOMPUTER_ERROR_CODES_H
#define CARCOMPUTER_ERROR_CODES_H

// Assigned number states error priority
// Error message is defined in `display::show_error_message`
#define ERROR_PEDAL_DISCONNECTED 1
#define ERROR_SPI_FAILED (1 << 1)
#define ERROR_CRASH_NO_ICE (1 << 2)
#define ERROR_GPS_TIMEOUT (1 << 3)
#define ERROR_SMS_FAILED (1 << 4)
#define ERROR_SD_FULL (1 << 5)

#endif //CARCOMPUTER_ERROR_CODES_H
