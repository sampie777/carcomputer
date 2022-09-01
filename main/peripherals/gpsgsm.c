//
// Created by samuel on 23-8-22.
//

#include <hal/uart_types.h>
#include <esp_check.h>
#include <driver/uart.h>
#include <hal/gpio_types.h>
#include <string.h>
#include "gpsgsm.h"
#include "../utils.h"
#include "display/display.h"

#define MESSAGE_MAX_LENGTH 128

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

enum ConnectionState {
    Initializing = 0,
    Initialized,
    GpsEnableRequestSent,
    GpsEnableRequestApproved,
    GpsEnableNmeaLoggingRequestSent,
    GpsEnableNmeaLoggingRequestApproved,
    GpsNmeaLoggingStarted,
};
static enum ConnectionState connection_state = Initializing;

enum SmsState {
    Idle = 0,
    Sending,
    SentSuccess,
    SentFailed
};
static enum SmsState sms_state = Idle;

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

void convert_gngga_message(State *state, const char *message) {
    NmeaGNGGAMessage gga_message = {0};
    extract_GNGGA_message(message, &gga_message);
    if (nmea_calculate_checksum(message) != gga_message.checksum) {
        // Checksum failed
        return;
    }

    state->location.latitude = nmea_coordinates_to_degrees(gga_message.latitude, gga_message.latitude_direction);
    state->location.longitude = nmea_coordinates_to_degrees(gga_message.longitude, gga_message.longitude_direction);
    state->location.altitude = gga_message.altitude;
    state->location.quality = gga_message.quality;
    state->location.satellites = gga_message.satellites;

    int hours = (int) (gga_message.timestamp / 10000);
    state->location.time.minutes = (int) (gga_message.timestamp / 100) - hours * 100;
    state->location.time.seconds = (int) gga_message.timestamp - hours * 10000 - state->location.time.minutes * 100;
    // Adjust for timezone
    hours += state->location.time.timezone;
    if (hours > 23) {
        hours -= 24;
    } else if (hours < 0) {
        hours += 24;
    }
    state->location.time.hours = hours;
}

void convert_gnrmc_message(State *state, const char *message) {
    NmeaGNRMCMessage rmc_message = {0};
    extract_GNRMC_message(message, &rmc_message);
    if (nmea_calculate_checksum(message) != rmc_message.checksum) {
        // Checksum failed
        return;
    }

    state->location.is_effective_positioning = rmc_message.status == 'A';
    state->location.ground_speed = rmc_message.ground_speed * 1.852;    // knots -> km/h
    state->location.ground_heading = rmc_message.ground_heading;

    uint8_t day = rmc_message.date / 10000;
    uint8_t month = rmc_message.date / 100 - day * 100;
    uint16_t year = 2000 + rmc_message.date - day * 10000 - month * 100;

    if (year > 2021 && year < 2079) {
        state->location.time.day = day;
        state->location.time.month = month;
        state->location.time.year = year;
    }
}

void transmit(const char *data, uint8_t with_break) {
    printf("[GPS] Sending\n");
    if (with_break) {
        uart_write_bytes_with_break(GPSGSM_UART_NUMBER, data, strlen(data), 50);
    } else {
        uart_write_bytes(GPSGSM_UART_NUMBER, data, strlen(data));
    }
}

/**
 * Send the data string in smaller pieces at a time to reduce riskt for lengthy transfers
 * @param data
 * @param max_transfer_size
 */
void transmit_safe(const char *data, size_t max_transfer_size, uint8_t with_break) {
    printf("[GPS] Sending safe\n");
    size_t length = strlen(data);
    for (size_t i = 0; i < length; i += max_transfer_size) {
        if (with_break && i + max_transfer_size > length) {
            uart_write_bytes_with_break(GPSGSM_UART_NUMBER, &data[i], min(max_transfer_size, length - i), 50);
        } else {
            uart_write_bytes(GPSGSM_UART_NUMBER, &data[i], min(max_transfer_size, length - i));
        }
    }
}

void enable_internet() {
    printf("[GPS] Enable internet\n");
    transmit("AT+CGATT=1\r", true);
    connection_state = GpsEnableRequestSent;
}

void enable_gps() {
    printf("[GPS] Enable GPS and AGPS\n");
    transmit("AT+GPS=1\r", true);
//    transmit("AT+AGPS=0\r");
//    transmit("AT+AGPS=1\r");
    connection_state = GpsEnableRequestSent;
}

void enable_gps_logging() {
    printf("[GPS] Enable GPS logging\n");
    delay_ms(10);
    transmit("AT+GPSRD=1\r", true);
    connection_state = GpsEnableNmeaLoggingRequestSent;
}

void process_message(State *state, const char *message) {
    if (strlen(message) == 0) {
        return;
    }

    if (!starts_with(message, "$")) {
        printf("[GPS] Processing: '%s' with length: %d\n", message, strlen(message));
    }

    if (strcmp(message, "Init...") == 0) {
        connection_state = Initializing;
    } else if (starts_with(message, "+CIEV") || starts_with(message, "+CREG")) {
        connection_state = Initialized;
    } else if (connection_state == GpsEnableRequestSent && strcmp(message, "OK") == 0) {
        connection_state = GpsEnableRequestApproved;
    } else if (connection_state == GpsEnableNmeaLoggingRequestSent && strcmp(message, "OK") == 0) {
        connection_state = GpsEnableNmeaLoggingRequestApproved;
    } else if (starts_with(message, "+CMGS=")) {
        sms_state = SentSuccess;
    }

    if (strstr(message, "$GNGGA") != NULL) {
        connection_state = GpsNmeaLoggingStarted;
        convert_gngga_message(state, message);
    } else if (strstr(message, "$GNRMC") != NULL) {
        convert_gnrmc_message(state, message);
    }
}

void read_messages(State *state) {
    static char *last_message;
    static int last_message_index = 0;

    if (last_message == NULL) {
        last_message = malloc(MESSAGE_MAX_LENGTH);
    }

    // Read data from UART.
    int length = 0;
    ESP_ERROR_CHECK(uart_get_buffered_data_len(GPSGSM_UART_NUMBER, (size_t *) &length));
    if (length == 0) {
        return;
    }

    char data[MESSAGE_MAX_LENGTH];
    length = uart_read_bytes(GPSGSM_UART_NUMBER, data, min(length, MESSAGE_MAX_LENGTH), 100);

    for (int i = 0; i < length; i++) {
        if (last_message_index >= MESSAGE_MAX_LENGTH) {
            printf("[GPS] Max message length reached\n");
            last_message[MESSAGE_MAX_LENGTH - 1] = '\0';
            process_message(state, last_message);
            last_message_index = 0;
            continue;
        }

        if (data[i] == '\n') continue;
        if (data[i] == '\r') {
            last_message[last_message_index] = '\0';
            process_message(state, last_message);
            last_message_index = 0;
            continue;
        }

        last_message[last_message_index++] = data[i];
    }
}

void gpsgsm_process(State *state) {
    static int64_t state_start_time = 0;
    static int64_t sms_sent_time = 0;
    static enum ConnectionState previous_state = 0xff;

    if (connection_state != previous_state) {
        state_start_time = esp_timer_get_time_ms();
        previous_state = connection_state;
    }

    if (connection_state == Initializing && esp_timer_get_time_ms() > state_start_time + GPSGSM_INIT_MAX_TIMEOUT_MS) {
        printf("[GPS] Init timeout\n");
        connection_state = Initialized;
    } else if (connection_state == Initialized) {
        enable_internet();
        enable_gps();
    } else if (connection_state == GpsEnableRequestApproved
               || (connection_state == GpsEnableRequestSent && esp_timer_get_time_ms() > state_start_time + 500)) {
        enable_gps_logging();
    } else if (connection_state != GpsNmeaLoggingStarted && esp_timer_get_time_ms() > state_start_time + GPSGSM_MESSAGE_MAX_TIMEOUT_MS) {
        printf("[GPS] Initial NMEA message timeout\n");
        display_set_error_message(state, "GPS timeout");
        // Retry GPS enabling
        connection_state = Initialized;
    }

    if (sms_state == Sending) {
        if (sms_sent_time == 0) {
            sms_sent_time = esp_timer_get_time_ms();
        } else if (esp_timer_get_time_ms() > sms_sent_time + GPSGSM_SMS_SENT_MAX_TIMEOUT_MS) {
            sms_state = SentFailed;
        }
    } else if (sms_state == SentSuccess) {
        sms_state = Idle;
        sms_sent_time = 0;
    } else if (sms_state == SentFailed) {
        display_set_error_message(state, "SMS failed");
        sms_state = Idle;
        sms_sent_time = 0;
    }

    read_messages(state);

    state->location.is_gps_on = connection_state == GpsNmeaLoggingStarted;
}

void gpsgsm_init() {
    printf("[GPS] Initializing...\n");
    connection_state = Initializing;

    uart_config_t uart_config = {
            .baud_rate = GPSGSM_UART_BAUD_RATE,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .rx_flow_ctrl_thresh = 122,
    };
    // Configure UART parameters
    ESP_ERROR_CHECK(uart_param_config(GPSGSM_UART_NUMBER, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(GPSGSM_UART_NUMBER, GPSGSM_UART_TX_PIN, GPSGSM_UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    // Setup UART buffered IO with event queue
    const int uart_buffer_size = (1024 * 2);
    QueueHandle_t uart_queue;
    // Install UART driver using an event queue here
    ESP_ERROR_CHECK(uart_driver_install(GPSGSM_UART_NUMBER, uart_buffer_size, \
                                        uart_buffer_size, 10, &uart_queue, 0));

    printf("[GPS] Init done\n");
}

void gsm_send_sms(const char *number, const char *message) {
    printf("[GSM] Sending SMS to %s with content: '%s'\n", number, message);

    char buffer[32];
    // Enable text mode
    transmit("AT+CMGF=1\r", true);
    delay_ms(100);

    // Start SMS to number
    sprintf(buffer, "AT+CMGS=%s\r", number);
    transmit(buffer, true);
    delay_ms(500);

    // Insert SMS message
    transmit_safe(message, 8, false);
    transmit("\r", true);
    delay_ms(500);

    // Send SMS
    sprintf(buffer, "%c\r", 0x1a);
    transmit(buffer, true);
    sms_state = Sending;
}

void gsm_http_post(const char *url, const char *json) {
    printf("[GSM] Sending data to %s\n", url);

    // Strip domain from url
    char url_copy[strlen(url)];
    strcpy(url_copy, url);
    strtok(url_copy, "://");
    char *domain = strtok(NULL, "/");

    char *json_escaped;
    string_escape(json, &json_escaped);
    printf("[GSM] JSON %s\n", json_escaped);

    transmit("AT+CGATT=1\r", false);
    transmit_safe("AT+CIPSTART=\"TCP\",\"", 8, false);
    transmit_safe(domain, 8, false);
    transmit("\",80\r", true);

    // Cave man way to wait for the connection to be established
    delay_ms(5000);
    uart_flush(GPSGSM_UART_NUMBER);

    transmit("AT+HTTPPOST=\"", false);
    transmit_safe(url, 8, false);
    transmit("\",\"application/json\",\"", false);
    transmit_safe(json, 8, false);
    transmit("\"\r", true);

    free(json_escaped);
}