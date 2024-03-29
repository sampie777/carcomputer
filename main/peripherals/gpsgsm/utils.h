//
// Created by samuel on 15-11-22.
//

#ifndef CARCOMPUTER_UTILS_H
#define CARCOMPUTER_UTILS_H

#include <stdbool.h>
#include "definitions.h"
#include "../../state.h"

void extract_uint16(char **source, uint16_t *destination, char *delimiter);

void extract_uint8(char **source, uint8_t *destination, char *delimiter);

void extract_int8(char **source, int8_t *destination, char *delimiter);

void extract_int(char **source, int *destination, char *delimiter);

void extract_double(char **source, double *destination, char *delimiter);

void extract_char(char **source, char *destination, char *delimiter);

void extract_string(char **source, char **destination, char *delimiter);

void extract_hex(char **source, unsigned int *destination, char *delimiter);

void extract_GNGGA_message(const char *string, NmeaGNGGAMessage *message);

void extract_GNRMC_message(const char *string, NmeaGNRMCMessage *message);

int nmea_calculate_checksum(const char *message);

double nmea_coordinates_to_degrees(double coordinates, char direction);

void extract_ctzv_message(const char *message, Time *time);

bool a9g_state_compare(A9GState *a, A9GState *b);

void a9g_state_clone(A9GState *source, A9GState *destination);

void a9g_state_reset(A9GState *source);

#endif //CARCOMPUTER_UTILS_H