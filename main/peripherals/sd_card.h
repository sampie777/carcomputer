//
// Created by samuel on 18-8-22.
//

#ifndef APP_TEMPLATE_SD_CARD_H
#define APP_TEMPLATE_SD_CARD_H

#include "../state.h"

// Max directory/device name length is 32 chars (+ 1 for null terminator)
#define SD_DIRECTORY_MAX_LENGTH (32 + 1)

int sd_card_init();
int sd_card_deinit();
void sd_card_close_file();
void sd_card_delete_file(const char *file_name);
int sd_card_rename_file(char *file_name, const char *new_file_name);
int sd_card_file_append(const char *file_name, const char *line);
int sd_card_create_file_incremental(BootState *boot_state, const char *directory, const char *base_file_name, const char *base_file_extension,
                                    char *file_name_out);

#endif //APP_TEMPLATE_SD_CARD_H
