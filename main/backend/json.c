//
// Created by samuel on 25-11-22.
//

#include "json.h"
#include <string.h>
#include <stdio.h>

enum JsonType {
    JSON_NOTHING,
    JSON_TAG,
    JSON_VALUE_STRING,
};

void json_parser_on_tag(const char *tag, const char *value, RegistrationResponse *output) {
    printf("Tag: '%s' value: '%s'\n", tag, value);
    if (strcmp(tag, "access_token") == 0) {
        output->access_token = malloc(strlen(value) + 1);
        strcpy(output->access_token, value);
    } else if (strcmp(tag, "device_name") == 0) {
        output->device_name = malloc(strlen(value) + 1);
        strcpy(output->device_name, value);
    }
}

void json_parser_determine_next_tag(char *buffer, size_t buffer_length, char c, enum JsonType *current_type, enum JsonType *next_type) {
    switch (*next_type) {
        case JSON_TAG:
            if (c == '"' || c == '\'') {
                memset(buffer, 0, buffer_length);
                *current_type = JSON_TAG;
            } else if (c == '}') {
                *next_type = JSON_NOTHING;
            }
            break;
        case JSON_VALUE_STRING:
            if (c == '"' || c == '\'') {
                memset(buffer, 0, buffer_length);
                *current_type = JSON_VALUE_STRING;
            }
            break;
        case JSON_NOTHING:
        default:
            if (c == '{' || c == ',') {
                *current_type = JSON_NOTHING;
                *next_type = JSON_TAG;
            }
    }
}

void json_parser_process_current_tag(char *buffer, char c, char **tag, const enum JsonType *current_type, enum JsonType *next_type, RegistrationResponse *output) {
    switch (*current_type) {
        case JSON_TAG:
            if (c == '"' || c == '\'') {
                *tag = realloc(*tag, strlen(buffer) + 1);
                strcpy(*tag, buffer);
                *next_type = JSON_VALUE_STRING;
            } else {
                buffer[strlen(buffer)] = c;
            }
            break;
        case JSON_VALUE_STRING:
            if (c == '"' || c == '\'') {
                json_parser_on_tag(*tag, buffer, output);
                *next_type = JSON_NOTHING;
            } else {
                buffer[strlen(buffer)] = c;
            }
            break;
        default:
            break;
    }
}

size_t expand_buffer(char **buffer, size_t new_length) {
    *buffer = realloc(*buffer, new_length);
    memset(*buffer + strlen(*buffer), 0, new_length - strlen(*buffer));
    return new_length;
}

void parse_json_to_registration_response(const char *json, RegistrationResponse *output) {
    enum JsonType current_type = JSON_TAG;  // Something different from next_type
    enum JsonType next_type = JSON_NOTHING;
    size_t buffer_length = 32;
    char *buffer = calloc(buffer_length, sizeof(char));
    char *tag = NULL;

    for (int i = 0; i < strlen(json); i++) {
        char c = json[i];

        // Expand buffer with more zeros
        if (strlen(buffer) >= buffer_length - 2) {
            buffer_length = expand_buffer(&buffer, buffer_length + 32);
        }

        if (current_type != next_type) {
            json_parser_determine_next_tag(buffer, buffer_length, c, &current_type, &next_type);
        } else {
            json_parser_process_current_tag(buffer, c, &tag, &current_type, &next_type, output);
        }
    }

    free(tag);
    free(buffer);
}