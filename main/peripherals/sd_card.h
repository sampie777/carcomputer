//
// Created by samuel on 18-8-22.
//

#ifndef APP_TEMPLATE_SD_CARD_H
#define APP_TEMPLATE_SD_CARD_H

#include "../state.h"

int sd_card_init();
void sd_card_close_file();
void sd_card_delete_file(const char *file_name);
long sd_card_get_file_size(const char *file_name);
long sd_card_read_file(const char *file_name, char **content);
long sd_card_read_file_part(const char *file_name, long chunk_start, long chunk_size, char **content);
void list_files_on_sd_card(const char *directory);
int sd_card_rename_file(char *file_name, const char *new_file_name);
int sd_card_file_append(const char *file_name, const char *line);
int sd_card_create_file_incremental(BootState *boot_state, const char *directory, const char *base_file_name, const char *base_file_extension,
                                    char *file_name_out);

#endif //APP_TEMPLATE_SD_CARD_H
