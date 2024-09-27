//
// Created by samuel on 15-11-22.
//

#include <string.h>
#include <stdio.h>
#include "utils.h"

void extract_uint16(char **source, uint16_t *destination, char *delimiter) {
    char *match = strsep(source, delimiter);
    if (match == NULL) return;
    *destination = (uint16_t) strtol(match, NULL, 10);
}

void extract_uint8(char **source, uint8_t *destination, char *delimiter) {
    char *match = strsep(source, delimiter);
    if (match == NULL) return;
    *destination = (uint8_t) strtol(match, NULL, 10);
}

void extract_int8(char **source, int8_t *destination, char *delimiter) {
    char *match = strsep(source, delimiter);
    if (match == NULL) return;
    *destination = (int8_t) strtol(match, NULL, 10);
}

void extract_int(char **source, int *destination, char *delimiter) {
    char *match = strsep(source, delimiter);
    if (match == NULL) return;
    *destination = (int) strtol(match, NULL, 10);
}

void extract_double(char **source, double *destination, char *delimiter) {
    char *match = strsep(source, delimiter);
    if (match == NULL) return;
    *destination = strtod(match, NULL);
}

void extract_char(char **source, char *destination, char *delimiter) {
    char *match = strsep(source, delimiter);
    if (match == NULL) return;
    sscanf(match, "%c", destination);
}

void extract_string(char **source, char **destination, char *delimiter) {
    char *match = strsep(source, delimiter);
    if (match == NULL) return;
    char *result = malloc(strlen(*source));
    sscanf(match, "%s", result);
    if (destination != NULL) {
        *destination = malloc(strlen(result));
        strcpy(*destination, result);
    }
    free(result);
}

void extract_hex(char **source, unsigned int *destination, char *delimiter) {
    char *match = strsep(source, delimiter);
    if (match == NULL) return;
    *destination = strtoul(match, NULL, 16);
}

void extract_GNGGA_message(const char *string, NmeaGNGGAMessage *message) {
    char *search_string = malloc(strlen(string) + 1);
    char *search_string_pointer = search_string;
    strcpy(search_string, string);

    extract_string(&search_string, NULL, ",");
    extract_double(&search_string, &message->timestamp, ",");
    extract_double(&search_string, &message->latitude, ",");
    extract_char(&search_string, &message->latitude_direction, ",");
    extract_double(&search_string, &message->longitude, ",");
    extract_char(&search_string, &message->longitude_direction, ",");
    extract_int(&search_string, &message->quality, ",");
    extract_int(&search_string, &message->satellites, ",");
    extract_double(&search_string, &message->hdop, ",");
    extract_double(&search_string, &message->altitude, ",");
    extract_char(&search_string, &message->unit, ",");
    extract_double(&search_string, &message->geoidal_separation, ",");
    extract_char(&search_string, &message->geoidal_separation_unit, ",");
    extract_double(&search_string, &message->correction_age, ",");
    extract_int(&search_string, &message->station_id, "*");
    extract_hex(&search_string, &message->checksum, "*");

    free(search_string_pointer);
}

void extract_GNRMC_message(const char *string, NmeaGNRMCMessage *message) {
    char *search_string = malloc(strlen(string) + 1);
    char *search_string_pointer = search_string;
    strcpy(search_string, string);

    extract_string(&search_string, NULL, ",");
    extract_double(&search_string, &message->timestamp, ",");
    extract_char(&search_string, &message->status, ",");
    extract_double(&search_string, &message->latitude, ",");
    extract_char(&search_string, &message->latitude_direction, ",");
    extract_double(&search_string, &message->longitude, ",");
    extract_char(&search_string, &message->longitude_direction, ",");
    extract_double(&search_string, &message->ground_speed, ",");
    extract_double(&search_string, &message->ground_heading, ",");
    extract_int(&search_string, &message->date, ",");
    extract_double(&search_string, &message->declination, ",");
    extract_char(&search_string, &message->declination_direction, ",");
    extract_char(&search_string, &message->mode, "*");
    extract_hex(&search_string, &message->checksum, "*");

    free(search_string_pointer);
}

int nmea_calculate_checksum(const char *message) {
    int crc = 0;
    char *next = strstr(message, "$") + 1;
    while (*next != '*' && *next != '\0') {
        crc ^= *next;
        next++;
    }
    return crc;
}

double nmea_coordinates_to_degrees(double coordinates, char direction) {
    int degrees = (int) (coordinates / 100);
    double minutes = coordinates - degrees * 100;
    double result = degrees + minutes / 60.0;
    if (direction == 'S' || direction == 'W')
        result *= -1;
    return result;
}

void extract_ctzv_message(const char *message, Time *time) {
    char *search_string = malloc(strlen(message) + 1);
    char *search_string_pointer = search_string;
    strcpy(search_string, message);

    extract_string(&search_string, NULL, ":");
    extract_uint16(&search_string, &(time->year), "/");
    time->year += 2000;
    extract_uint8(&search_string, &(time->month), "/");
    extract_uint8(&search_string, &(time->day), ",");
    extract_uint8(&search_string, &(time->hours), ":");
    extract_uint8(&search_string, &(time->minutes), ":");
    extract_uint8(&search_string, &(time->seconds), ",");
    extract_int8(&search_string, &(time->timezone), "\0");

    free(search_string_pointer);
}

bool a9g_state_compare(A9GState *a, A9GState *b) {
    return a->initialized == b->initialized
           && a->network_attached == b->network_attached
           && a->pnp_parameters_set == b->pnp_parameters_set
           && a->pnp_activated == b->pnp_activated
           && a->agps_enabled == b->agps_enabled
           && a->gps_logging_enabled == b->gps_logging_enabled
           && a->gps_logging_started == b->gps_logging_started;
}

void a9g_state_clone(A9GState *source, A9GState *destination) {
    destination->initialized = source->initialized;
    destination->network_attached = source->network_attached;
    destination->pnp_parameters_set = source->pnp_parameters_set;
    destination->pnp_activated = source->pnp_activated;
    destination->agps_enabled = source->agps_enabled;
    destination->gps_logging_enabled = source->gps_logging_enabled;
    destination->gps_logging_started = source->gps_logging_started;
}

void a9g_state_reset(A9GState *source) {
    source->initialized = A9Status_Unknown;
    source->network_attached = A9Status_Unknown;
    source->pnp_parameters_set = A9Status_Unknown;
    source->pnp_activated = A9Status_Unknown;
    source->agps_enabled = A9Status_Unknown;
    source->gps_logging_enabled = A9Status_Unknown;
    source->gps_logging_started = false;
    source->network_error_count = 0;
}